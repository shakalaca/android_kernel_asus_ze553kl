#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>

static struct proc_dir_entry *project_id_proc_file;
static int project_id_proc_read(struct seq_file *buf, void *v)
{
	seq_printf(buf, "%d\n", asus_project_id);
	return 0;
}

static int project_id_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, project_id_proc_read, NULL);
}


static struct file_operations project_id_proc_ops = {
	.open = project_id_proc_open,
	.read = seq_read,
	.release = single_release,
};

static void create_project_id_proc_file(void)
{
    printk("create_project_id_proc_file\n");
    project_id_proc_file = proc_create("apid", 0444,NULL, &project_id_proc_ops);
    if(project_id_proc_file){
        printk("create project_id_proc_file sucessed!\n");
    }else{
	printk("create project_id_proc_file failed!\n");
    }
}


static struct proc_dir_entry *project_hw_proc_file;
static int project_hw_proc_read(struct seq_file *buf, void *v)
{
	seq_printf(buf, "%d\n", asus_hw_id);
	return 0;
}

static int project_hw_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, project_hw_proc_read, NULL);
}


static struct file_operations project_hw_proc_ops = {
	.open = project_hw_proc_open,
	.read = seq_read,
	.release = single_release,
};

static void create_project_hw_proc_file(void)
{
    printk("create_project_stage_proc_file\n");
    project_hw_proc_file = proc_create("apsta", 0444,NULL, &project_hw_proc_ops);
    if(project_hw_proc_file){
        printk("create project_stage_proc_file sucessed!\n");
    }else{
	printk("create project_stage_proc_file failed!\n");
    }
}

static struct proc_dir_entry *project_mp_proc_file;
static int project_mp_proc_read(struct seq_file *buf, void *v)
{
	seq_printf(buf, "%d\n", asus_mp_id);
	return 0;
}

static int project_mp_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, project_mp_proc_read, NULL);
}


static struct file_operations project_mp_proc_ops = {
	.open = project_mp_proc_open,
	.read = seq_read,
	.release = single_release,
};

static void create_project_mp_proc_file(void)
{
    printk("create_mp_proc_file\n");
    project_mp_proc_file = proc_create("mpid", 0444,NULL, &project_mp_proc_ops);
    if(project_mp_proc_file){
        printk("create mp_proc_file sucessed!\n");
    }else{
	printk("create mp_proc_file failed!\n");
    }
}

//<ASUS-Jessie_Tian-20160617>create node rf_id+++
static struct proc_dir_entry *project_rf_proc_file;
static int project_rf_proc_read(struct seq_file *buf, void *v)
{
	seq_printf(buf, "%d\n", asus_rf_id);
	return 0;
}

static int project_rf_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, project_rf_proc_read, NULL);
}


static struct file_operations project_rf_proc_ops = {
	.open = project_rf_proc_open,
	.read = seq_read,
	.release = single_release,
};

static void create_project_rf_proc_file(void)
{
    printk("create_project_rf_proc_file\n");
    project_rf_proc_file = proc_create("aprf", 0444,NULL, &project_rf_proc_ops);
    if(project_rf_proc_file){
        printk("create project_rf_proc_file sucessed!\n");
    }else{
	printk("create project_rf_proc_file failed!\n");
    }
}
//<ASUS-Jessie_Tian-20160617>create node rf_id---

static struct proc_dir_entry *asus_fp_id_proc_file;

static int asus_fpid_proc_read(struct seq_file *buf, void *v)
{
	seq_printf(buf, "%d\n", asus_fp_id);
	return 0;
}

static int asus_fpid_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, asus_fpid_proc_read, NULL);
}

static struct file_operations asus_fp_proc_ops = {
	.open = asus_fpid_proc_open,
	.read = seq_read,
	.release = single_release,
};

static void create_asus_fp_id_proc_file(void)
{
	 printk("create_asus_fp_id_proc_file\n");
	 asus_fp_id_proc_file = proc_create("afpid",0444,NULL,&asus_fp_proc_ops);
	 if(asus_fp_id_proc_file) {
	 	printk("create asus_fp_proc_ops sucessed!\n");
	 }else{
	 	printk("create_asus_fp_id_proc_file failed\n");
	 }
}

#define       CAMERA_FILE "camera_node"


static int camera_node_read(struct seq_file *buf, void *v)
{
	struct file *camerafile = NULL;
	mm_segment_t old_fs;
	unsigned char cambuf=0;

	if(camerafile == NULL)
			camerafile = filp_open("/factory/camerafile", O_RDWR,0);
   	if (IS_ERR(camerafile)) {
            printk("error occured while opening file camerafile, exiting...\n");
          	cambuf=0;
   	}
	else
	{
		old_fs = get_fs();
		set_fs(KERNEL_DS);
       	camerafile->f_op->read(camerafile, &cambuf, 1, &camerafile->f_pos);
       	set_fs(old_fs);
       	filp_close(camerafile, NULL);  
	}
	seq_printf(buf, "%d\n",cambuf);
	return 0;
}
static ssize_t camera_node_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	int temp;
	char messages[256];
	struct file *camerafile = NULL;
	mm_segment_t old_fs;	 

	if(len>256) len=256;
	if (copy_from_user(messages, buff, len)) return -EFAULT;
	sscanf(buff,"%d", &temp);
	
	if(camerafile == NULL)
    		camerafile = filp_open("/factory/camerafile", O_RDWR | O_CREAT, S_IRWXG|S_IRWXO|S_IRWXU);
	if (IS_ERR(camerafile)) {
		    printk("error occured while opening file camerafile, exiting...\n");
		    return 0;
	}
	   
	old_fs = get_fs();
	set_fs(KERNEL_DS);
   	camerafile->f_op->write(camerafile,(char *)&temp, 1, &camerafile->f_pos);
   	set_fs(old_fs);
   	filp_close(camerafile, NULL);  
	
	return len;
}
static int camera_node_open(struct inode *inode, struct  file *file)
{
	return single_open(file, camera_node_read, NULL);
}
static const struct file_operations camera_node_fops = {
	.owner = THIS_MODULE,
	.open =  camera_node_open,
	.write = camera_node_write,
	.read = seq_read,
	.release = single_release,
};
static void create_asus_camera_node_proc_file(void)
{
	struct proc_dir_entry *camera_node_entry = proc_create(CAMERA_FILE, 0777, NULL, &camera_node_fops);
	if (!camera_node_entry) printk("[camera]%s failed!\n",CAMERA_FILE);
}

#define	CAMERASINGLE_FILE "camerasingle_node"

static int camerasingle_node_read(struct seq_file *buf, void *v)
{
	struct file *camerasinglefile = NULL;
	mm_segment_t old_fs;
	unsigned char cambuf=0;
	 
	if(camerasinglefile == NULL)
    	camerasinglefile = filp_open("/factory/camerasinglefile", O_RDWR,0);

	if (IS_ERR(camerasinglefile)) {
		printk("error occured while opening file camerasinglefile, exiting...\n");
		cambuf=0;
	}
	else
	{
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		camerasinglefile->f_op->read(camerasinglefile, &cambuf, 1, &camerasinglefile->f_pos);
		set_fs(old_fs);
		filp_close(camerasinglefile, NULL);  
	}

	seq_printf(buf, "%d\n",cambuf);

	return 0;
}

static ssize_t camerasingle_node_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	int temp;
	char messages[256];
	struct file *camerasinglefile = NULL;
	mm_segment_t old_fs;	 

	if(len>256) len=256;
	if (copy_from_user(messages, buff, len)) return -EFAULT;
	sscanf(buff,"%d", &temp);
	
	if(camerasinglefile == NULL)
                camerasinglefile = filp_open("/factory/camerasinglefile", O_RDWR | O_CREAT, S_IRWXG|S_IRWXO|S_IRWXU);
	if (IS_ERR(camerasinglefile)) {
                printk("error occured while opening file camerasinglefile, exiting...\n");
                return 0;
	}
	   
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	camerasinglefile->f_op->write(camerasinglefile,(char *)&temp, 1, &camerasinglefile->f_pos);
	set_fs(old_fs);
	filp_close(camerasinglefile, NULL);  
	
	return len;
}

static int camerasingle_node_open(struct inode *inode, struct  file *file)
{
	return single_open(file, camerasingle_node_read, NULL);
}

static const struct file_operations camerasingle_node_fops = {
	.owner = THIS_MODULE,
	.open =  camerasingle_node_open,
	.write = camerasingle_node_write,
	.read = seq_read,
	.release = single_release,
};

static void create_asus_camerasingle_node_proc_file(void)
{
	struct proc_dir_entry *camerasingle_node_entry = proc_create(CAMERASINGLE_FILE, 0777, NULL, &camerasingle_node_fops);
	if (!camerasingle_node_entry) printk("[camera]%s failed!\n",CAMERASINGLE_FILE);
}

static int __init proc_asusPRJ_init(void)
{
	create_project_id_proc_file();
	create_project_hw_proc_file();
	create_project_rf_proc_file();
	create_asus_fp_id_proc_file();
	create_project_mp_proc_file();
	create_asus_camera_node_proc_file();
	create_asus_camerasingle_node_proc_file();
	return 0;
}
module_init(proc_asusPRJ_init);
