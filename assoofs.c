#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO  */
#include <linux/init.h>         /* Needed for the macros */
#include <linux/fs.h>           /* libfs stuff           */
#include <linux/buffer_head.h>  /* buffer_head           */
#include <linux/slab.h>         /* kmem_cache            */
#include "assoofs.h"

/*
 *  Operaciones sobre ficheros
 */

ssize_t assoofs_read(struct file * filp, char __user * buf, size_t len, loff_t * ppos);
ssize_t assoofs_write(struct file * filp, const char __user * buf, size_t len, loff_t * ppos);
static struct inode *assoofs_get_inode(struct super_block *sb, int ino);
int assoofs_sb_get_a_freeblock(struct super_block *sb, uint64_t *block);
void assoofs_save_sb_info(struct super_block *vsb);
int assoofs_save_inode_info(struct super_block *sb, struct assoofs_inode_info *inode_info);
void assoofs_add_inode_info(struct super_block *sb, struct assoofs_inode_info *inode);
struct assoofs_inode_info *assoofs_search_inode_info(struct super_block *sb, struct assoofs_inode_info *start, struct assoofs_inode_info *search);

const struct file_operations assoofs_file_operations = {
    .read = assoofs_read,
    .write = assoofs_write,
};

MODULE_LICENSE("GPL");



struct assoofs_inode_info *assoofs_get_inode_info(struct super_block *sb, uint64_t inode_no){

        struct assoofs_inode_info *inode_info = NULL;
        struct buffer_head *bh;
         struct assoofs_super_block_info *afs_sb = sb->s_fs_info;
        struct assoofs_inode_info *buffer = NULL;
        int i;
        bh = sb_bread(sb, ASSOOFS_INODESTORE_BLOCK_NUMBER);
        inode_info = (struct assoofs_inode_info *)bh->b_data;

       
        for (i = 0; i < afs_sb->inodes_count; i++) {
            if (inode_info->inode_no == inode_no) {
            buffer = kmalloc(sizeof(struct assoofs_inode_info), GFP_KERNEL);
            memcpy(buffer, inode_info, sizeof(*buffer));
                break;
            }
        inode_info++;
        }

        brelse(bh);
        return buffer;

}


ssize_t assoofs_read(struct file * filp, char __user * buf, size_t len, loff_t * ppos) {
    printk(KERN_INFO "Read request\n");
    return 0;
}

ssize_t assoofs_write(struct file * filp, const char __user * buf, size_t len, loff_t * ppos) {
    printk(KERN_INFO "Write request\n");
    return 0;
}

/*
 *  Operaciones sobre directorios
 */
static int assoofs_iterate(struct file *filp, struct dir_context *ctx);
const struct file_operations assoofs_dir_operations = {
    .owner = THIS_MODULE,
    .iterate = assoofs_iterate,
};


//Verificar que funciona la práctica una vez implementado el código
static int assoofs_iterate(struct file *filp, struct dir_context *ctx) {
    printk(KERN_INFO "Iterate request\n");
    return 0;
}

/*
 *  Operaciones sobre inodos
 */
static int assoofs_create(struct user_namespace *mnt_userns, struct inode *dir, struct dentry *dentry, umode_t mode, bool excl);
struct dentry *assoofs_lookup(struct inode *parent_inode, struct dentry *child_dentry, unsigned int flags);
static int assoofs_mkdir(struct user_namespace *mnt_userns, struct inode *dir , struct dentry *dentry, umode_t mode);
static struct inode_operations assoofs_inode_ops = {
    .create = assoofs_create,
    .lookup = assoofs_lookup,
    .mkdir = assoofs_mkdir,
};


struct assoofs_inode_info *assoofs_search_inode_info(struct super_block *sb, struct assoofs_inode_info *start, struct assoofs_inode_info *search){

    uint64_t count = 0;
    while (start->inode_no != search->inode_no && count < ((struct assoofs_super_block_info *)sb->s_fs_info)->inodes_count) {
        count++;
        start++;
   }

    if (start->inode_no == search->inode_no)
        return start;
    else
        return NULL;
}





//Sujeto a posibles fallos verificar 

int assoofs_save_inode_info(struct super_block *sb, struct assoofs_inode_info *inode_info){
    
  

    struct buffer_head *bh;
    struct assoofs_inode_info *inode_pos;

    printk(KERN_INFO "assoofs_save_inode_info");

    bh = sb_bread(sb, ASSOOFS_INODESTORE_BLOCK_NUMBER);
    inode_pos = assoofs_search_inode_info(sb, (struct assoofs_inode_info *)bh->b_data, inode_info);
   

    memcpy(inode_pos, inode_info, sizeof(*inode_pos));
    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    
    return 0;
}

//Posible fallo
void assoofs_add_inode_info(struct super_block *sb, struct assoofs_inode_info *inode){
    
    struct buffer_head *bh;
    struct assoofs_inode_info *inode_info;
    struct assoofs_super_block_info *assoofs_sb = sb->s_fs_info;
   

    printk(KERN_INFO "assoofs_add_inode_info");

    bh = sb_bread(sb, ASSOOFS_INODESTORE_BLOCK_NUMBER);
    inode_info = (struct assoofs_inode_info *)bh->b_data;
    inode_info += assoofs_sb->inodes_count;
    memcpy(inode_info, inode, sizeof(struct assoofs_inode_info));

    bh = sb_bread(sb, ASSOOFS_INODESTORE_BLOCK_NUMBER);

    mark_buffer_dirty(bh);
    
    sync_dirty_buffer(bh);


    assoofs_sb->inodes_count++;
    assoofs_save_sb_info(sb);
}


void assoofs_save_sb_info(struct super_block *vsb){
    
        struct buffer_head *bh;
        struct assoofs_super_block_info *sb = vsb->s_fs_info; // Informaci ́on persistente del superbloque en memoria
        bh = sb_bread(vsb, ASSOOFS_SUPERBLOCK_BLOCK_NUMBER);
        bh->b_data = (char *)sb; // Sobreescribo los datos de disco con la informaci ́on en memoria
        mark_buffer_dirty(bh);
        sync_dirty_buffer(bh);
        brelse(bh);
}



int assoofs_sb_get_a_freeblock(struct super_block *sb, uint64_t *block){
   
    struct assoofs_super_block_info *assoofs_sb = sb->s_fs_info;
    int i;
   
   printk(KERN_INFO "assofs_sb_get_a_freeblock Information");

   for (i = 2; i < ASSOOFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED; i++)
     if (assoofs_sb->free_blocks & (1 << i))
        break; // cuando aparece el primer bit 1 en free_block dejamos de recorrer el mapa de bits, i tiene la posici ́o del primer bloque libre
    
    
    if(*block>ASSOOFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED){
        printk(KERN_ERR "Número de bloques superior al límite");
        return -1;
    }


   *block = i; // Escribimos el valor de i en la direcci ́on de memoria indicada como segundo argumento en la función

    assoofs_sb->free_blocks &= ~(1 << i);
    assoofs_save_sb_info(sb);
    return 0;


}

struct dentry *assoofs_lookup(struct inode *parent_inode, struct dentry *child_dentry, unsigned int flags){

    
    struct assoofs_inode_info *parent_info = parent_inode->i_private;
    struct super_block *sb = parent_inode->i_sb;
    struct buffer_head *bh;
    struct assoofs_dir_record_entry *record;
    int i;

    printk(KERN_INFO "LookUp Information");

    bh = sb_bread(sb, parent_info->data_block_number);

    
    
    record = (struct assoofs_dir_record_entry *)bh->b_data;
    for (i=0; i < parent_info->dir_children_count; i++) {
         if (!strcmp(record->filename, child_dentry->d_name.name)) {
            struct inode *inode = assoofs_get_inode(sb, record->inode_no); // Función auxiliar que obtine la información de un inodo a partir de su n ́umero de inodo.
            inode_init_owner(sb->s_user_ns, inode, parent_inode, ((struct assoofs_inode_info *)inode->i_private)->mode);
            d_add(child_dentry, inode);
         return NULL;
        }
      record++;
    }
    printk(KERN_ERR "ERROR, INODE NOT FOUND Ought!!");
    return NULL;
}




static int assoofs_create(struct user_namespace *mnt_userns, struct inode *dir, struct dentry *dentry, umode_t mode, bool excl) {
    
    struct inode *inode;
    struct assoofs_inode_info *inode_info;
    struct assoofs_inode_info *parent_inode_info;
    struct assoofs_dir_record_entry *dir_contents;
    struct buffer_head *bh;
    uint64_t count;
    struct super_block *sb;

    printk(KERN_INFO "assofs_create Information");

    sb = dir->i_sb; // obtengo un puntero al superbloque desde dir
    
    count = ((struct assoofs_super_block_info *)sb->s_fs_info)->inodes_count; // obtengo el n ́umero de inodos de la informaci ́on persistente del superbloque
    if(count>ASSOOFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED){
        printk(KERN_ERR "ERROR EN EL NUMERO DE LOS INODOS");
        return -1;
    }

    inode = new_inode(sb);
    inode->i_sb = sb;
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
    inode->i_op = &assoofs_inode_ops;
    inode->i_ino = count + 1; // Asigno número al nuevo inodo a partir de count
    
    inode_info = kmalloc(sizeof(struct assoofs_inode_info), GFP_KERNEL);
    inode_info->inode_no = inode->i_ino;
    inode_info->mode = mode; // El segundo mode me llega como argumento
    inode_info->file_size = 0;
    inode->i_private = inode_info;

    inode->i_fop=&assoofs_file_operations;
    
    inode_init_owner(sb->s_user_ns, inode, dir, mode);
    d_add(dentry, inode);
    
    assoofs_sb_get_a_freeblock(sb, &inode_info->data_block_number);
    
    assoofs_add_inode_info(sb, inode_info);
    
    parent_inode_info = dir->i_private;
    bh = sb_bread(sb, parent_inode_info->data_block_number);

    dir_contents = (struct assoofs_dir_record_entry *)bh->b_data;
    dir_contents += parent_inode_info->dir_children_count;
    dir_contents->inode_no = inode_info->inode_no; // inode_info es la informaci ́on persistente del inodo creado en el paso 2.

    strcpy(dir_contents->filename, dentry->d_name.name);
    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
    
    parent_inode_info->dir_children_count++;
    assoofs_save_inode_info(sb, parent_inode_info);
    
    return 0;
}


//Sujeto a fallos, verificar detenidamente

static int assoofs_mkdir(struct user_namespace *mnt_userns, struct inode *dir , struct dentry *dentry, umode_t mode) {
    
    
    struct inode *inode;
    struct assoofs_inode_info *inode_info;
    struct assoofs_inode_info *parent_inode_info;
    struct assoofs_dir_record_entry *dir_contents;
    struct buffer_head *bh;
    uint64_t count;
    struct super_block *sb;

    printk(KERN_INFO "assofs_create Information");

    sb = dir->i_sb; // obtengo un puntero al superbloque desde dir
    
    count = ((struct assoofs_super_block_info *)sb->s_fs_info)->inodes_count; // obtengo el n ́umero de inodos de la informaci ́on persistente del superbloque
    if(count>ASSOOFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED){
        printk(KERN_ERR "ERROR EN EL NUMERO DE LOS INODOS");
        return -1;
    }

    inode = new_inode(sb);
    inode->i_sb = sb;
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
    inode->i_op = &assoofs_inode_ops;
    inode->i_ino = count + 1; // Asigno número al nuevo inodo a partir de count
    
    inode_info = kmalloc(sizeof(struct assoofs_inode_info), GFP_KERNEL);
    inode_info->inode_no = inode->i_ino;
    inode_info->mode = S_IFDIR | mode; // El segundo mode me llega como argumento
    inode_info->dir_children_count = 0;
    inode->i_private = inode_info;
    

    inode->i_fop=&assoofs_dir_operations;
    
    inode_init_owner(sb->s_user_ns, inode, dir, mode);
    d_add(dentry, inode);
    
    assoofs_sb_get_a_freeblock(sb, &inode_info->data_block_number);
    
    assoofs_add_inode_info(sb, inode_info);
    
    parent_inode_info = dir->i_private;
    bh = sb_bread(sb, parent_inode_info->data_block_number);

    dir_contents = (struct assoofs_dir_record_entry *)bh->b_data;
    dir_contents += parent_inode_info->dir_children_count;
    dir_contents->inode_no = inode_info->inode_no; // inode_info es la informaci ́on persistente del inodo creado en el paso 2.

    strcpy(dir_contents->filename, dentry->d_name.name);
    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
    
    parent_inode_info->dir_children_count++;
    assoofs_save_inode_info(sb, parent_inode_info);
    
     
    return 0;
}

/*
 *  Operaciones sobre el superbloque
 */
static const struct super_operations assoofs_sops = {
    .drop_inode = generic_delete_inode,
};

/*
 *  Inicialización del superbloque
 */
int assoofs_fill_super(struct super_block *sb, void *data, int silent) {   
    // 1.- Leer la información persistente del superbloque del dispositivo de bloques
    struct buffer_head *bh;
    struct assoofs_super_block_info *assoofs_sb;
    struct inode *root_inode=NULL;
    
    printk(KERN_INFO "assoofs_fill_super request\n"); 

    bh = sb_bread(sb, ASSOOFS_SUPERBLOCK_BLOCK_NUMBER); // sb lo recibe assoofs_fill_super como argumento
    assoofs_sb = (struct assoofs_super_block_info *)bh->b_data;
    brelse(bh);
    if(assoofs_sb->magic!=ASSOOFS_MAGIC && assoofs_sb->block_size!=ASSOOFS_DEFAULT_BLOCK_SIZE){
            printk(KERN_ERR "Error during register\n");
            return -1;
    }

    sb->s_magic = ASSOOFS_MAGIC;
    sb->s_maxbytes = ASSOOFS_DEFAULT_BLOCK_SIZE;
    sb->s_op=&assoofs_sops;
    sb->s_fs_info=assoofs_sb;
    
    root_inode = new_inode(sb);
    inode_init_owner(sb->s_user_ns, root_inode, NULL, S_IFDIR); // S_IFDIR para directorios, S_IFREG para ficheros.
    
    root_inode->i_ino = ASSOOFS_ROOTDIR_INODE_NUMBER; // número de inodo
    root_inode->i_sb = sb; // puntero al superbloque
    root_inode->i_op = &assoofs_inode_ops; // direcci ́on de una variable de tipo struct inode_operations previamente declarada
    root_inode->i_fop = &assoofs_dir_operations; /* direcci ́on de una variable de tipo struct file_operations previamente declarada. 
    En la pr ́actica tenemos 2: assoofs_dir_operations y assoofs_file_operations. La primera la
    utilizaremos cuando creemos inodos para directorios (como el directorio ra ́ız) y la segunda cuando creemos
    inodos para ficheros.*/
    root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = current_time(root_inode); // fechas.
    root_inode->i_private = assoofs_get_inode_info(sb, ASSOOFS_ROOTDIR_INODE_NUMBER); // Informaci ́on persistente del inodo
    sb->s_root = d_make_root(root_inode);

    

    // 2.- Comprobar los parámetros del superbloque
    
    
    
    // 3.- Escribir la información persistente leída del dispositivo de bloques en el superbloque sb, incluído el campo s_op con las operaciones que soporta.
    
    
    // 4.- Crear el inodo raíz y asignarle operaciones sobre inodos (i_op) y sobre directorios (i_fop)
    return 0;
}

/*
 *  Montaje de dispositivos assoofs
 */
static struct dentry *assoofs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data) {
    struct dentry *ret;
    printk(KERN_INFO "assoofs_mount request\n");
    ret = mount_bdev(fs_type, flags, dev_name, data, assoofs_fill_super);
    // Control de errores a partir del valor de ret. En este caso se puede utilizar la macro IS_ERR: if (IS_ERR(ret)) ...
    return ret;
}

/*
 *  assoofs file system type
 */
static struct file_system_type assoofs_type = {
    .owner   = THIS_MODULE,
    .name    = "assoofs",
    .mount   = assoofs_mount,
    .kill_sb = kill_litter_super,
};

static int __init assoofs_init(void) {
    int ret;
    printk(KERN_INFO "assoofs_init request\n");
    ret = register_filesystem(&assoofs_type);
    // Control de errores a partir del valor de ret
    if(ret!=0){
    	printk(KERN_ERR "Error during register");
    }
    return ret;
}

static void __exit assoofs_exit(void) {
    int ret;
    printk(KERN_INFO "assoofs_exit request\n");
    ret = unregister_filesystem(&assoofs_type);
    // Control de errores a partir del valor de ret
     if(ret!=0){
    	printk(KERN_ERR "Error during register");
    }
}

//lookup y get_inode
static struct inode *assoofs_get_inode(struct super_block *sb, int ino){
    
    struct inode *inode;
    struct assoofs_inode_info *inode_info = NULL;
    printk(KERN_INFO "assofs_get_inode Information");
    inode_info = assoofs_get_inode_info(sb, ino);
    inode=new_inode(sb);
    inode->i_ino = ino; // n ́umero de inodo
    inode->i_sb = sb; // puntero al superbloque
    inode->i_op = &assoofs_inode_ops; // direcci ́on de una variable de tipo struct inode_operations previamente declarada    

    if (S_ISDIR(inode_info->mode))
        inode->i_fop = &assoofs_dir_operations;
    else if (S_ISREG(inode_info->mode))
        inode->i_fop = &assoofs_file_operations;
    else
        printk(KERN_ERR "Unknown inode type. Neither a directory nor a file.");

    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode); // fechas.
    inode->i_private = assoofs_get_inode_info(sb, ASSOOFS_ROOTDIR_INODE_NUMBER); // Informaci ́on persistente del inodo

    return inode;
}







module_init(assoofs_init);
module_exit(assoofs_exit);
