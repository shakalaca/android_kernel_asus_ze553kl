#include <vfsSpiDrv.h>

#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <asm/uaccess.h>


#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/input.h>
#include <linux/fb.h>
#include <linux/notifier.h>

#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/timer.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/i2c/twl.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/irq.h>
#include <linux/compat.h>

#include <asm-generic/siginfo.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/jiffies.h>

#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>

#include <linux/pinctrl/consumer.h>
#include <linux/clk.h>
#include <soc/qcom/scm.h>
#include <soc/qcom/qseecomi.h>
#include <linux/proc_fs.h>

//herman poll wait start
#include <linux/poll.h>
//herman poll wait end

#include <linux/wakelock.h>

#include <linux/completion.h>
#define reinit_completion(x) INIT_COMPLETION(*(x))

struct fp_device_data {
	/* +++ common part +++ */
	struct regulator *sovcc;
	struct regulator *vcc;
	unsigned int drdy_pin; // int_gpio
	unsigned int isr_pin;	// isr
	unsigned int sleep_pin; // rst_gpio
	unsigned int osvcc_pin;
	unsigned int pwr_gpio;
	unsigned int module_vendor;

	/* Wakelock Protect */
	struct wake_lock wake_lock;
	bool irq_wakeup_flag;
	/* Wakelock Protect */
	/* --- common part --- */

	/* +++ SYNA +++ */
	struct device sndev;
	dev_t devt;
	struct cdev cdev;
	spinlock_t vfs_spi_lock;
	struct list_head device_entry;
	struct mutex buffer_mutex;
	unsigned int is_opened;
	unsigned char *buffer;
	unsigned char *null_buffer;
	unsigned char *stream_buffer;
	size_t stream_buffer_size;
#if DO_CHIP_SELECT
	unsigned int cs_pin;
#endif
	int user_pid;
	int signal_id;
	unsigned int current_spi_speed;
	unsigned int is_drdy_irq_enabled;
	unsigned int drdy_ntf_type;
	struct mutex kernel_lock;
	/* --- SYNA --- */	
};

#define vendor_module_syna 1
#define vendor_module_elan 2
#define vendor_module_eos 3
static int g_module_vendor;

/* +++ Synaptics part +++ */
#define VALIDITY_PART_NAME "validity_fingerprint"
static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_mutex);
static struct class *vfsspi_device_class;
static int gpio_irq;
#ifdef VFSSPI_32BIT
/*
 * Used by IOCTL compat command:
 *         VFSSPI_IOCTL_RW_SPI_MESSAGE
 *
 * @rx_buffer:pointer to retrieved data
 * @tx_buffer:pointer to transmitted data
 * @len:transmitted/retrieved data size
 */
struct vfsspi_compat_ioctl_transfer {
	compat_uptr_t rx_buffer;
	compat_uptr_t tx_buffer;
	unsigned int len;
};
#endif
static int vfsspi_sendDrdyEventFd(struct fp_device_data *vfsSpiDev);
static int vfsspi_sendDrdyNotify(struct fp_device_data *vfsSpiDev);
/* --- Synaptics part --- */



/* +++ Synaptics operate zone +++ */
#define ELAN_CALI_TIMEOUT_MSEC	10000
struct completion cmd_done;

static int vfsspi_send_drdy_signal(struct fp_device_data *vfsspi_device)
{
	struct task_struct *t;
	int ret = 0;

	pr_debug("vfsspi_send_drdy_signal\n");

	if (vfsspi_device->user_pid != 0) {
		rcu_read_lock();
		/* find the task_struct associated with userpid */
		pr_debug("Searching task with PID=%08x\n",
			vfsspi_device->user_pid);
		t = pid_task(find_pid_ns(vfsspi_device->user_pid, &init_pid_ns),
			     PIDTYPE_PID);
		if (t == NULL) {
			pr_debug("No such pid\n");
			rcu_read_unlock();
			return -ENODEV;
		}
		rcu_read_unlock();
		/* notify DRDY signal to user process */
		ret = send_sig_info(vfsspi_device->signal_id,
				    (struct siginfo *)1, t);
		if (ret < 0)
			pr_err("Error sending signal\n");

	} else {
		pr_err("pid not received yet\n");
	}

	return ret;
}
#if 0
/* Return no. of bytes written to device. Negative number for errors */
static inline ssize_t vfsspi_writeSync(struct fp_device_data *vfsspi_device,
					size_t len)
{
	int    status = 0;
	struct spi_message m;
	struct spi_transfer t;

	pr_debug("vfsspi_writeSync\n");

	spi_message_init(&m);
	memset(&t, 0, sizeof(t));

	t.rx_buf = vfsspi_device->null_buffer;
	t.tx_buf = vfsspi_device->buffer;
	t.len = len;
	t.speed_hz = vfsspi_device->current_spi_speed;

	spi_message_add_tail(&t, &m);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 0);
#endif
	status = spi_sync(vfsspi_device->spi, &m);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 1);
#endif
	if (status == 0)
		status = m.actual_length;
	pr_debug("vfsspi_writeSync,length=%d\n", status);
	return status;
}

/* Return no. of bytes read > 0. negative integer incase of error. */
static inline ssize_t vfsspi_readSync(struct fp_device_data *vfsspi_device,
					size_t len)
{
	int    status = 0;
	struct spi_message m;
	struct spi_transfer t;

	pr_debug("vfsspi_readSync\n");

	spi_message_init(&m);
	memset(&t, 0x0, sizeof(t));

	memset(vfsspi_device->null_buffer, 0x0, len);
	t.tx_buf = vfsspi_device->null_buffer;
	t.rx_buf = vfsspi_device->buffer;
	t.len = len;
	t.speed_hz = vfsspi_device->current_spi_speed;

	spi_message_add_tail(&t, &m);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 0);
#endif
	status = spi_sync(vfsspi_device->spi, &m);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 1);
#endif
	if (status == 0)
		status = len;

	pr_debug("vfsspi_readSync,length=%d\n", status);

	return status;
}

static ssize_t vfsspi_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *fPos)
{
	struct fp_device_data *vfsspi_device = NULL;
	ssize_t               status = 0;

	pr_debug("vfsspi_write\n");

	if (count > DEFAULT_BUFFER_SIZE || count <= 0)
		return -EMSGSIZE;

	vfsspi_device = filp->private_data;

	mutex_lock(&vfsspi_device->buffer_mutex);

	if (vfsspi_device->buffer) {
		unsigned long missing = 0;

		missing = copy_from_user(vfsspi_device->buffer, buf, count);

		if (missing == 0)
			status = vfsspi_writeSync(vfsspi_device, count);
		else
			status = -EFAULT;
	}

	mutex_unlock(&vfsspi_device->buffer_mutex);

	return status;
}

static ssize_t vfsspi_read(struct file *filp, char __user *buf,
			size_t count, loff_t *fPos)
{
	struct fp_device_data *vfsspi_device = NULL;
	ssize_t                status    = 0;

	pr_debug("vfsspi_read\n");

	if (count > DEFAULT_BUFFER_SIZE || count <= 0)
		return -EMSGSIZE;
	if (buf == NULL)
		return -EFAULT;


	vfsspi_device = filp->private_data;

	mutex_lock(&vfsspi_device->buffer_mutex);

	status  = vfsspi_readSync(vfsspi_device, count);


	if (status > 0) {
		unsigned long missing = 0;
		/* data read. Copy to user buffer.*/
		missing = copy_to_user(buf, vfsspi_device->buffer, status);

		if (missing == status) {
			pr_err("copy_to_user failed\n");
			/* Nothing was copied to user space buffer. */
			status = -EFAULT;
		} else {
			status = status - missing;
		}
	}

	mutex_unlock(&vfsspi_device->buffer_mutex);

	return status;
}

static int vfsspi_xfer(struct fp_device_data *vfsspi_device,
			struct vfsspi_ioctl_transfer *tr)
{
	int status = 0;
	struct spi_message m;
	struct spi_transfer t;

	pr_debug("vfsspi_xfer\n");

	if (vfsspi_device == NULL || tr == NULL)
		return -EFAULT;

	if (tr->len > DEFAULT_BUFFER_SIZE || tr->len <= 0)
		return -EMSGSIZE;

	if (tr->tx_buffer != NULL) {

		if (copy_from_user(vfsspi_device->null_buffer,
				tr->tx_buffer, tr->len) != 0)
			return -EFAULT;
	}

	spi_message_init(&m);
	memset(&t, 0, sizeof(t));

	t.tx_buf = vfsspi_device->null_buffer;
	t.rx_buf = vfsspi_device->buffer;
	t.len = tr->len;
	t.speed_hz = vfsspi_device->current_spi_speed;

	spi_message_add_tail(&t, &m);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 0);
#endif
	status = spi_sync(vfsspi_device->spi, &m);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 1);
#endif
	if (status == 0) {
		if (tr->rx_buffer != NULL) {
			unsigned missing = 0;

			missing = copy_to_user(tr->rx_buffer,
					       vfsspi_device->buffer, tr->len);

			if (missing != 0)
				tr->len = tr->len - missing;
		}
	}
	pr_debug("vfsspi_xfer,length=%d\n", tr->len);
	return status;

} /* vfsspi_xfer */

static int vfsspi_rw_spi_message(struct fp_device_data *vfsspi_device,
				 unsigned long arg)
{
	struct vfsspi_ioctl_transfer   *dup  = NULL;
#ifdef VFSSPI_32BIT
    struct vfsspi_compat_ioctl_transfer   dup_compat;
#endif	
	dup = kmalloc(sizeof(struct vfsspi_ioctl_transfer), GFP_KERNEL);
	if (dup == NULL)
		return -ENOMEM;
#ifdef VFSSPI_32BIT
	if (copy_from_user(&dup_compat, (void __user *)arg,
			   sizeof(struct vfsspi_compat_ioctl_transfer)) != 0)  {
#else
	if (copy_from_user(dup, (void __user *)arg,
			   sizeof(struct vfsspi_ioctl_transfer)) != 0)  {
#endif
		return -EFAULT;
	} else {
		int err;
#ifdef VFSSPI_32BIT		
		dup->rx_buffer = (unsigned char *)(unsigned long)dup_compat.rx_buffer;
		dup->tx_buffer = (unsigned char *)(unsigned long)dup_compat.tx_buffer;
		dup->len = dup_compat.len;
#endif
		err = vfsspi_xfer(vfsspi_device, dup);
		if (err != 0) {
			kfree(dup);
			return err;
		}
	}
#ifdef VFSSPI_32BIT
    dup_compat.len = dup->len;
	if (copy_to_user((void __user *)arg, &dup_compat,
			 sizeof(struct vfsspi_compat_ioctl_transfer)) != 0){
#else
	if (copy_to_user((void __user *)arg, dup,
			 sizeof(struct vfsspi_ioctl_transfer)) != 0){
#endif
		kfree(dup);
		return -EFAULT;
	}
	kfree(dup);
	return 0;
}

static int vfsspi_set_clk(struct fp_device_data *vfsspi_device,
			  unsigned long arg)
{
	unsigned short clock = 0;
	struct spi_device *spidev = NULL;

	if (copy_from_user(&clock, (void __user *)arg,
			   sizeof(unsigned short)) != 0)
		return -EFAULT;

	spin_lock_irq(&vfsspi_device->vfs_spi_lock);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 0);
#endif
	spidev = spi_dev_get(vfsspi_device->spi);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 1);
#endif
	spin_unlock_irq(&vfsspi_device->vfs_spi_lock);
	if (spidev != NULL) {
		switch (clock) {
		case 0:	/* Running baud rate. */
			pr_debug("Running baud rate.\n");
			spidev->max_speed_hz = MAX_BAUD_RATE;
			vfsspi_device->current_spi_speed = MAX_BAUD_RATE;
			break;
		case 0xFFFF: /* Slow baud rate */
			pr_debug("slow baud rate.\n");
			spidev->max_speed_hz = SLOW_BAUD_RATE;
			vfsspi_device->current_spi_speed = SLOW_BAUD_RATE;
			break;
		default:
			pr_debug("baud rate is %d.\n", clock);
			vfsspi_device->current_spi_speed =
				clock * BAUD_RATE_COEF;
			if (vfsspi_device->current_spi_speed > MAX_BAUD_RATE)
				vfsspi_device->current_spi_speed =
					MAX_BAUD_RATE;
			spidev->max_speed_hz = vfsspi_device->current_spi_speed;
			break;
		}
		spi_dev_put(spidev);
	}
	return 0;
}
#endif
static int vfsspi_register_drdy_signal(struct fp_device_data *vfsspi_device,
				       unsigned long arg)
{
	struct vfsspi_ioctl_register_signal usr_signal;
	if (copy_from_user(&usr_signal, (void __user *)arg, sizeof(usr_signal)) != 0) {
		pr_err("Failed copy from user.\n");
		return -EFAULT;
	} else {
		vfsspi_device->user_pid = usr_signal.user_pid;
		vfsspi_device->signal_id = usr_signal.signal_id;
	}
	return 0;
}

static irqreturn_t vfsspi_irq(int irq, void *context)
{
	struct fp_device_data *vfsspi_device = context;
	/* Linux kernel is designed so that when you disable
	an edge-triggered interrupt, and the edge happens while
	the interrupt is disabled, the system will re-play the
	interrupt at enable time.
	Therefore, we are checking DRDY GPIO pin state to make sure
	if the interrupt handler has been called actually by DRDY
	interrupt and it's not a previous interrupt re-play */
	if (gpio_get_value(vfsspi_device->drdy_pin) == DRDY_ACTIVE_STATUS) {
		wake_lock_timeout(&vfsspi_device->wake_lock, 1000);
		vfsspi_sendDrdyNotify(vfsspi_device);
	}

	return IRQ_HANDLED;
}

static int vfsspi_sendDrdyEventFd(struct fp_device_data *vfsSpiDev)
{
    struct task_struct *t;
    struct file *efd_file = NULL;
    struct eventfd_ctx *efd_ctx = NULL;	int ret = 0;

    pr_debug("vfsspi_sendDrdyEventFd\n");

    if (vfsSpiDev->user_pid != 0) {
        rcu_read_lock();
        /* find the task_struct associated with userpid */
        pr_debug("Searching task with PID=%08x\n", vfsSpiDev->user_pid);
        t = pid_task(find_pid_ns(vfsSpiDev->user_pid, &init_pid_ns),
            PIDTYPE_PID);
        if (t == NULL) {
            pr_debug("No such pid\n");
            rcu_read_unlock();
            return -ENODEV;
        }
        efd_file = fcheck_files(t->files, vfsSpiDev->signal_id);
        rcu_read_unlock();

        if (efd_file == NULL) {
            pr_debug("No such efd_file\n");
            return -ENODEV;
        }
        
        efd_ctx = eventfd_ctx_fileget(efd_file);
        if (efd_ctx == NULL) {
            pr_debug("eventfd_ctx_fileget is failed\n");
            return -ENODEV;
        }

        /* notify DRDY eventfd to user process */
        eventfd_signal(efd_ctx, 1);

        /* Release eventfd context */
        eventfd_ctx_put(efd_ctx);
    }

    return ret;
}

static int vfsspi_sendDrdyNotify(struct fp_device_data *vfsSpiDev)
{
    int ret = 0;

    if (vfsSpiDev->drdy_ntf_type == VFSSPI_DRDY_NOTIFY_TYPE_EVENTFD) {
        ret = vfsspi_sendDrdyEventFd(vfsSpiDev);
    } else {
        ret = vfsspi_send_drdy_signal(vfsSpiDev);
    }

    return ret;
}

static int vfsspi_enableIrq(struct fp_device_data *vfsspi_device)
{
	pr_debug("vfsspi_enableIrq\n");

	if (vfsspi_device->is_drdy_irq_enabled == DRDY_IRQ_ENABLE) {
		pr_debug("DRDY irq already enabled\n");
		return -EINVAL;
	}

	enable_irq(gpio_irq);
	vfsspi_device->is_drdy_irq_enabled = DRDY_IRQ_ENABLE;

	return 0;
}

static int vfsspi_disableIrq(struct fp_device_data *vfsspi_device)
{
	pr_debug("vfsspi_disableIrq\n");

	if (vfsspi_device->is_drdy_irq_enabled == DRDY_IRQ_DISABLE) {
		pr_debug("DRDY irq already disabled\n");
		return -EINVAL;
	}

	disable_irq_nosync(gpio_irq);
	vfsspi_device->is_drdy_irq_enabled = DRDY_IRQ_DISABLE;

	return 0;
}
static int vfsspi_set_drdy_int(struct fp_device_data *vfsspi_device,
			       unsigned long arg)
{
	unsigned short drdy_enable_flag;
	if (copy_from_user(&drdy_enable_flag, (void __user *)arg,
			   sizeof(drdy_enable_flag)) != 0) {
		pr_err("Failed copy from user.\n");
		return -EFAULT;
	}
	if (drdy_enable_flag == 0)
			vfsspi_disableIrq(vfsspi_device);
	else {
			vfsspi_enableIrq(vfsspi_device);
			/* Workaround the issue where the system
			  misses DRDY notification to host when
			  DRDY pin was asserted before enabling
			  device.*/
			if (gpio_get_value(vfsspi_device->drdy_pin) ==
				DRDY_ACTIVE_STATUS) {
				vfsspi_sendDrdyNotify(vfsspi_device);
			}
	}
	return 0;
}


static void vfsspi_hardReset(struct fp_device_data *vfsspi_device)
{
	pr_debug("vfsspi_hardReset\n");
	printk("[Jacob] vfsspi sleep pin = %d \n", vfsspi_device->sleep_pin);
	if (vfsspi_device != NULL) {
		gpio_set_value(vfsspi_device->sleep_pin, 0);
		mdelay(1);
		gpio_set_value(vfsspi_device->sleep_pin, 1);
		mdelay(5);
	}
}


static void vfsspi_suspend(struct fp_device_data *vfsspi_device)
{
	pr_debug("vfsspi_suspend\n");
	printk("[Jacob] vfsspi sleep pin suspend\n");

	if (vfsspi_device != NULL) {
		spin_lock(&vfsspi_device->vfs_spi_lock);
		gpio_set_value(vfsspi_device->sleep_pin, 0);
		spin_unlock(&vfsspi_device->vfs_spi_lock);
	}
}

static long vfsspi_ioctl(struct file *filp, unsigned int cmd,
			unsigned long arg)
{
	int ret_val = 0;
	struct fp_device_data *vfsspi_device = NULL;

	pr_debug("vfsspi_ioctl\n");

	if (_IOC_TYPE(cmd) != VFSSPI_IOCTL_MAGIC) {
		pr_err("invalid magic. cmd=0x%X Received=0x%X Expected=0x%X\n",
			cmd, _IOC_TYPE(cmd), VFSSPI_IOCTL_MAGIC);
		return -ENOTTY;
	}

	vfsspi_device = filp->private_data;
	
	mutex_lock(&vfsspi_device->buffer_mutex);
	switch (cmd) {
	case VFSSPI_IOCTL_DEVICE_RESET:
		pr_debug("VFSSPI_IOCTL_DEVICE_RESET:\n");
		vfsspi_hardReset(vfsspi_device);
		break;
	case VFSSPI_IOCTL_DEVICE_SUSPEND:
	{
		pr_debug("VFSSPI_IOCTL_DEVICE_SUSPEND:\n");
		vfsspi_suspend(vfsspi_device);
		break;
	}		
#if 0
	case VFSSPI_IOCTL_RW_SPI_MESSAGE:
		pr_debug("VFSSPI_IOCTL_RW_SPI_MESSAGE");
		ret_val = vfsspi_rw_spi_message(vfsspi_device, arg);
		break;
	case VFSSPI_IOCTL_SET_CLK:
		pr_debug("VFSSPI_IOCTL_SET_CLK\n");
		ret_val = vfsspi_set_clk(vfsspi_device, arg);
		break;
#endif
	case VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL:
		pr_debug("VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL\n");
		ret_val = vfsspi_register_drdy_signal(vfsspi_device, arg);
		break;
	case VFSSPI_IOCTL_SET_DRDY_INT:
		pr_debug("VFSSPI_IOCTL_SET_DRDY_INT\n");
		ret_val = vfsspi_set_drdy_int(vfsspi_device, arg);
		break;
	case VFSSPI_IOCTL_SELECT_DRDY_NTF_TYPE:
        {
            vfsspi_iocSelectDrdyNtfType_t drdyTypes;

            pr_debug("VFSSPI_IOCTL_SELECT_DRDY_NTF_TYPE\n");

            if (copy_from_user(&drdyTypes, (void __user *)arg,
                sizeof(vfsspi_iocSelectDrdyNtfType_t)) != 0) {
                    pr_debug("copy from user failed.\n");
                    ret_val = -EFAULT;
            } else {
                if (0 != (drdyTypes.supportedTypes & VFSSPI_DRDY_NOTIFY_TYPE_EVENTFD)) {
                    vfsspi_device->drdy_ntf_type = VFSSPI_DRDY_NOTIFY_TYPE_EVENTFD;
                } else {
                    vfsspi_device->drdy_ntf_type = VFSSPI_DRDY_NOTIFY_TYPE_SIGNAL;
                }
                drdyTypes.selectedType = vfsspi_device->drdy_ntf_type;
                if (copy_to_user((void __user *)arg, &(drdyTypes),
                    sizeof(vfsspi_iocSelectDrdyNtfType_t)) == 0) {
                        ret_val = 0;
                } else {
                    pr_debug("copy to user failed\n");
                }
            }
            break;
        }
	default:
		ret_val = -EFAULT;
		break;
	}
	mutex_unlock(&vfsspi_device->buffer_mutex);
	return ret_val;
}

static int vfsspi_open(struct inode *inode, struct file *filp)
{
	struct fp_device_data *vfsspi_device = NULL;
	int status = -ENXIO;

	pr_debug("vfsspi_open\n");

	mutex_lock(&device_list_mutex);

	list_for_each_entry(vfsspi_device, &device_list, device_entry) {
		if (vfsspi_device->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}

	if (status == 0) {
		mutex_lock(&vfsspi_device->kernel_lock);
		if (vfsspi_device->is_opened != 0) {
			status = -EBUSY;
			pr_err("vfsspi_open: is_opened != 0, -EBUSY");
			goto vfsspi_open_out;
		}
		vfsspi_device->user_pid = 0;
        vfsspi_device->drdy_ntf_type = VFSSPI_DRDY_NOTIFY_TYPE_SIGNAL;
		if (vfsspi_device->buffer != NULL) {
			pr_err("vfsspi_open: buffer != NULL");
			goto vfsspi_open_out;
		}
		vfsspi_device->null_buffer =
			kmalloc(DEFAULT_BUFFER_SIZE, GFP_KERNEL);
		if (vfsspi_device->null_buffer == NULL) {
			status = -ENOMEM;
			pr_err("vfsspi_open: null_buffer == NULL, -ENOMEM");
			goto vfsspi_open_out;
		}
		vfsspi_device->buffer =
			kmalloc(DEFAULT_BUFFER_SIZE, GFP_KERNEL);
		if (vfsspi_device->buffer == NULL) {
			status = -ENOMEM;
			kfree(vfsspi_device->null_buffer);
			pr_err("vfsspi_open: buffer == NULL, -ENOMEM");
			goto vfsspi_open_out;
		}
		vfsspi_device->is_opened = 1;
		filp->private_data = vfsspi_device;
		nonseekable_open(inode, filp);

vfsspi_open_out:
		mutex_unlock(&vfsspi_device->kernel_lock);
	}
	mutex_unlock(&device_list_mutex);
	return status;
}


static int vfsspi_release(struct inode *inode, struct file *filp)
{
	struct fp_device_data *vfsspi_device = NULL;
	int                   status     = 0;

	pr_debug("vfsspi_release\n");

	mutex_lock(&device_list_mutex);
	vfsspi_device = filp->private_data;
	filp->private_data = NULL;
	vfsspi_device->is_opened = 0;
	if (vfsspi_device->buffer != NULL) {
		kfree(vfsspi_device->buffer);
		vfsspi_device->buffer = NULL;
	}

	if (vfsspi_device->null_buffer != NULL) {
		kfree(vfsspi_device->null_buffer);
		vfsspi_device->null_buffer = NULL;
	}

	mutex_unlock(&device_list_mutex);
	return status;
}

/* file operations associated with device */
static const struct file_operations vfsspi_fops = {
	.owner   = THIS_MODULE,
//	.write   = vfsspi_write,
//	.read    = vfsspi_read,
	.unlocked_ioctl   = vfsspi_ioctl,
	.open    = vfsspi_open,
	.release = vfsspi_release,
};

/* --- Synaptics operate zone --- */

/* +++ common operate zone +++ */

/* +++ ASUS proc fingerprint Interface +++ */
static int fingerpring_proc_read(struct seq_file *buf, void *v)
{

	if(g_module_vendor == 1) {
		return seq_printf(buf, "%s\n", "SN");
	} else if (g_module_vendor == 2) {
		return seq_printf(buf, "%s\n", "EL");
	} else if (g_module_vendor == 3) {
		return seq_printf(buf, "%s\n", "EO");
	} else {
		return seq_printf(buf, "%s\n", "NA");
	}

}
static int fingerprint_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, fingerpring_proc_read, NULL);
}

static void create_fingerprint_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  fingerprint_proc_open,
		.read = seq_read,
	};
	struct proc_dir_entry *proc_file = proc_create("fpmod", 0444, NULL, &proc_fops);

	if (!proc_file) {
		printk("[PF]%s failed!\n", __FUNCTION__);
	}
	return;
}
/* --- ASUS proc fingerprint Interface --- */
#if 0
static bool check_elan_module(struct fp_device_data *pdata)
{
	char tx_buf[64] = {1};
	char rx_buf[64] = {1};
	struct spi_transfer t;
	struct spi_message m;
	int i, j = 0;

	printk("spi_test_transfer start ! \n");
	for (j = 0; j < 2; j++) {
		mdelay(20);
		for (i=0; i<6; i++) {
			printk("%0x ", rx_buf[i]);
		}
		printk("\n");
		tx_buf[0] = 0xC1;	//EP0 Read
		tx_buf[1] = 1;
		pdata->spi->bits_per_word = 8;
		pdata->spi->mode  = SPI_MODE_0 ;
		memset(&t, 0, sizeof(t));
		t.tx_buf = tx_buf;
		t.rx_buf = rx_buf;
		t.len = 6;

		printk("[jacob] spi_message_init ! \n");
		spi_message_init(&m);
		printk("[jacob] spi_message_add_tail! \n");
		spi_message_add_tail(&t, &m);
		printk("[jacob] spi_message_add_tail end ! \n");
		printk("spi sync return %d \n", spi_sync(pdata->spi, &m));

	}

	if(0xbf == rx_buf[3] && 0x37 == rx_buf[5]) {
		IMG_WIDTH = (unsigned int)(rx_buf[5] - rx_buf[4] + 1);
		IMG_HEIGHT = (unsigned int)(rx_buf[3] - rx_buf[2] + 1);
		IMG_WIDTH_DEFAULT = IMG_WIDTH;
		IMG_HEIGHT_DEFAULT = IMG_HEIGHT;

		return true;
	} else {
		return false;
	}

}

static bool check_syna_module(struct fp_device_data *pdata)
{
	char tx_buf[64] = {1};
	char rx_buf[64] = {1};
	struct spi_transfer t;
	struct spi_message m;
	int i, j = 0;

	mdelay(20);

	printk("spi_test_transfer start ! \n");
	for (j = 0; j < 2; j++) {
		for (i=0; i<6; i++) {
			printk("%0x ", rx_buf[i]);
		}
		printk("\n");
		tx_buf[0] = 1;	//EP0 Read
		tx_buf[1] = 0;
		pdata->spi->bits_per_word = 8;
		pdata->spi->mode  = SPI_MODE_0 ;
		pdata->spi->max_speed_hz = MAX_BAUD_RATE;
		memset(&t, 0, sizeof(t));
		t.tx_buf = tx_buf;
		t.rx_buf = rx_buf;
		t.len = 6;

		printk("[jacob] spi_message_init ! \n");
		spi_message_init(&m);
		printk("[jacob] spi_message_add_tail! \n");
		spi_message_add_tail(&t, &m);
		printk("[jacob] spi_message_add_tail end ! \n");
		printk("spi sync return %d \n", spi_sync(pdata->spi, &m));

		for (i=0; i<6; i++) {
			printk("%0x ", rx_buf[i]);
		}
		printk("\n");
		mdelay(10);

		printk("spi_test_transfer start  01 \n");

		tx_buf[0] = 1;	//EP0 Read
		tx_buf[1] = 0;

		memset(&t, 0, sizeof(t));
		t.tx_buf = tx_buf;
		t.rx_buf = rx_buf;
		t.len = 6;

		spi_message_init(&m);
		spi_message_add_tail(&t, &m);
		printk("spi sync return %d \n", spi_sync(pdata->spi, &m));

		for (i=0; i<6; i++) {
			printk("%0x ", rx_buf[i]);
		}
		printk("\n");
		mdelay(5);

		printk("spi_test_transfer start  02 \n");

		tx_buf[0] = 2;	//EP0 Read
		tx_buf[1] = 1;

		memset(&t, 0, sizeof(t));
		t.tx_buf = tx_buf;
		t.rx_buf = rx_buf;
		t.len = 2;

		spi_message_init(&m);
		spi_message_add_tail(&t, &m);
		spi_sync(pdata->spi, &m);

		mdelay(5);

		printk("spi_test_transfer start  03 \n");

		tx_buf[0] = 3;	//EP0 Read
		tx_buf[1] = 0;

		memset(&t, 0, sizeof(t));
		t.tx_buf = tx_buf;
		t.rx_buf = rx_buf;
		t.len = 40;

		spi_message_init(&m);
		spi_message_add_tail(&t, &m);
		printk("spi_test_transfer start  03 \n");
		printk("spi sync return %d \n", spi_sync(pdata->spi, &m));

		for (i=0; i<40; i++) {
			printk("%0x ", rx_buf[i]);
		}
		printk("\n");
	}

	if((0xff == rx_buf[0] || 0x7f == rx_buf[0]) && 0xff == rx_buf[1] && 0x0 == rx_buf[2] && 0x0 == rx_buf[3]) {
		return true;
	} else {
		return false;
	}

}

/* vfsspi pinctrol */
static int fp_set_pinctrl(struct fp_device_data *pdata, bool active)
{
	int ret;
	if (active) { /* Change to active settings */
		ret = pinctrl_select_state(pdata->pinctrl, pdata->pins_active);
	}else {
		ret = pinctrl_select_state(pdata->pinctrl, pdata->pins_sleep);
	}

	if (ret)
		dev_err(&pdata->spi->dev, "%s: pinctrl_select_state ret:%d Setting:%d\n", __func__, ret, active);

	return ret;
}
/* vfsspi pinctrol */

/* vfsspi_set_clks */
static int fp_set_clks(struct fp_device_data *pdata, bool enable)
{
	int ret = 0;
	if (enable) {
		if (!fp_set_clks_flag) {
			/* Enable the spi clocks */
			ret = clk_set_rate(pdata->core_clk, pdata->spi->max_speed_hz); 
			if (ret) {
				dev_err(&pdata->spi->dev, "%s: Error setting clk_rate:%d\n", __func__, pdata->spi->max_speed_hz);
			}
			ret = clk_prepare_enable(pdata->core_clk);
			if (ret) {
				dev_err(&pdata->spi->dev, "%s: Error enabling core clk\n",  __func__);
			}
			ret = clk_prepare_enable(pdata->iface_clk);
			if (ret) {
				dev_err(&pdata->spi->dev, "%s: Error enabling iface clk\n", __func__);
			}
			fp_set_clks_flag = true;
		}
	} else {
		if (fp_set_clks_flag) {
			/* Disable the clocks */
			clk_disable_unprepare(pdata->iface_clk);
			clk_disable_unprepare(pdata->core_clk);
			fp_set_clks_flag = false;
		}
	}
	return ret; 
}
/* vfsspi_set_clks */

/* fvsspi_change_pipe_owner */
static int fp_change_pipe_owner(struct fp_device_data *pdata, bool to_tz)
{
	/* CMD ID collected from tzbsp_blsp.c to change Ownership */
	const u32 TZ_BLSP_MODIFY_OWNERSHIP_ID = 3;
	const u32 TZBSP_APSS_ID = 1;
	const u32 TZBSP_TZ_ID = 3 ;
	struct scm_desc desc; //scm call descriptor
	int ret;
	/* CMD ID to change BAM PIPE Owner*/
	desc.arginfo = SCM_ARGS(2); //# of arguments
	desc.args[0] = pdata->qup_id; //BLSPID (1-12)
	desc.args[1] = (to_tz) ? TZBSP_TZ_ID : TZBSP_APSS_ID; //Owner if TZ or Apps
	ret = scm_call2(SCM_SIP_FNID(SCM_SVC_TZ, TZ_BLSP_MODIFY_OWNERSHIP_ID), &desc);
	return (ret || desc.ret[0]) ? -1 : 0;
}
/* fvsspi_change_pipe_owner */

/* fvsspi_set_fabric */
static int fp_set_fabric(struct fp_device_data *pdata, bool active)
{
	int ret;
	struct spi_master *master = pdata->spi->master;
	
	if (active) {
		if (!fp_set_fabric_flag) {
			ret = master->prepare_transfer_hardware(master);
			fp_set_fabric_flag = true;
		}
	}else {
		if (fp_set_fabric_flag) {
			ret = master->unprepare_transfer_hardware(master);
			fp_set_fabric_flag = false;
		}
	}
	return ret;
}
/* fvsspi_set_fabric */
#endif

/* +++ ASUS add for suspend/resume +++ */
static int fp_sensor_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct fp_device_data *fp = dev_get_drvdata(&pdev->dev);

		printk("[FP] sensor suspend +++\n");
//		bState = 0;


		if (fp->is_drdy_irq_enabled & !fp->irq_wakeup_flag) {
			printk("[FP] enable irq wake up  \n");
			enable_irq_wake(fp->isr_pin);
			fp->irq_wakeup_flag = true;
		}


		printk("[FP] sensor suspend ---\n");
	
	return 0;
}

static int fp_sensor_resume(struct platform_device *pdev)
{
	struct fp_device_data *fp = dev_get_drvdata(&pdev->dev);

		printk("[FP] sensor resume +++\n");

		if (fp->irq_wakeup_flag) {
			disable_irq_wake(fp->isr_pin);
			fp->irq_wakeup_flag = false;
		}

#if 0
		if(InNW) {
			printk("[FP] sensor resume to NW \n");
			fp_set_pinctrl(fp, 1);
			fp_set_fabric(fp, 1);
			fp_set_clks(fp, 1);
			fp_change_pipe_owner(fp, 0);
		} else {
			printk("[FP] sensor resume to QSEE \n");
			fp_set_pinctrl(fp, 1);
			fp_set_fabric(fp, 1);
			fp_set_clks(fp, 1);
			fp_change_pipe_owner(fp, 1);
		}
#endif
//		bState = 1;
		
//		if (g_module_vendor == 2) {
//			elan_nw_module(fp);
//		}
//		gpio_set_value(fp->sleep_pin, 1);
		printk("[FP] sensor resume ---\n");
		
	return 0;
}

/* --- ASUS add for suspend/resume  --- */


/* +++ change excute mode +++ */
#if 0
static int fp_change_excute_mode(struct fp_device_data *pdata, bool tz)
{

	printk("[FP] fp_change_excute_mode = %d !\n", tz);
	if (tz) {	
		fp_set_pinctrl(pdata, 1);
		fp_set_fabric(pdata, 1);

		fp_set_clks(pdata, 1);
		fp_change_pipe_owner(pdata, 1);
		InNW = false;
	} else {
		fp_set_pinctrl(pdata, 1);
		fp_set_fabric(pdata, 1);

		fp_set_clks(pdata, 1);
		fp_change_pipe_owner(pdata, 0);
		InNW = true;
	}
	return 0;
	

}
#endif
/* --- change excute mode --- */

/* +++ fp NW/TEE evn transfer +++ */
#if 0
static int fp_transfer_init(struct spi_device *spi,
			struct fp_device_data *pdata, bool on)
{

	int status = 0;

	if (on) {

		/* pinctrol part*/
		printk("[Jacob] vfsspi Get the pinctrl node \n");
		/* Get the pinctrl node */
		pdata->pinctrl = devm_pinctrl_get(&spi->dev);
		if (IS_ERR_OR_NULL(pdata->pinctrl)) {
		     dev_err(&spi->dev, "%s: Failed to get pinctrl\n", __func__);
		     return PTR_ERR(pdata->pinctrl);
		}
		
		printk("[Jacob] vfsspi Get the active setting \n");
		/* Get the active setting */
		pdata->pins_active = pinctrl_lookup_state(pdata->pinctrl,
		                            "spi_default");
		if (IS_ERR_OR_NULL(pdata->pins_active)) {
			dev_err(&spi->dev, "%s: Failed to get pinctrl state active\n", __func__);
			return PTR_ERR(pdata->pins_active);
		}

		printk("[Jacob] vfsspi Get the sleep setting \n");
		/* Get sleep settings */
		pdata->pins_sleep = pinctrl_lookup_state(pdata->pinctrl,
		                            "spi_sleep");
		if (IS_ERR_OR_NULL(pdata->pins_sleep)) {
			dev_err(&spi->dev, "%s: Failed to get pinctrl state sleep\n", __func__);
		return PTR_ERR(pdata->pins_sleep);
		}
		/* pinctrol part*/

		/* clock part */
		printk("[Jacob] vfsspi Get iface_clk info \n");
		/* Get iface_clk info */
		pdata->iface_clk = clk_get(&spi->dev, "vfsiface_clk");
		if (IS_ERR(pdata->iface_clk)) {
			dev_err(&spi->dev, "%s: Failed to get iface_clk %ld\n", __func__, PTR_ERR(pdata->iface_clk));
			return PTR_ERR(pdata->iface_clk);
		}
		printk("[Jacob] vfsspi Get core_clk info \n");
		
		/* Get core_clk info */
		pdata->core_clk = clk_get(&spi->dev, "vfscore_clk");
		if (IS_ERR(pdata->core_clk)) {
			dev_err(&spi->dev, "%s: Failed to get core_clk %p\n", __func__, pdata->core_clk);
			return PTR_ERR(pdata->core_clk);
		}
		/* clock part */

		printk("[Jacob] vfsspi Get QUP ID \n");
		/* Get the QUP ID (#1-12) */
		status = of_property_read_u32(spi->dev.of_node, "qcom,qup-id", &pdata->qup_id);
		if (status) {
			dev_err(&spi->dev, "Error getting qup_id\n");
			return status;
		}

		/* Grab SPI master lock for exclusive access
		call spi_bus_unlock to unlock the lock. */
		//	spi_bus_lock(spi->master);
		printk("[Jacob] vfsspi Get QUP ID end\n");
	}
	return 0;
}
#endif
/* --- fp NW/TEE evn transfer --- */

/* +++ fp gpio init +++ */
static int fp_gpio_init(struct fp_device_data *pdata)
{

	int err = 0;
	/* request part */
	if (gpio_request(pdata->drdy_pin, "fp_drdy") < 0) {
		err = -EBUSY;
		printk("[FP][%s] gpio request failed ! \n", __func__);
		return err;		
	}

	if (gpio_request(pdata->pwr_gpio, "synap_pwr")) {
		err = -EBUSY;
        printk("[FingerPrint] gpio_request pwr_gpio Error!\n");
        goto fp_gpio_pwr_pin_failed;
    }

	err = gpio_direction_output(pdata->pwr_gpio, 1);
	if (err < 0) {
		printk("[FP][%s] fp_gpio_pwr_pin_failed failed ! \n", __func__);
		err = -EBUSY;
		goto fp_gpio_pwr_pin_failed;
	}

	if (gpio_request(pdata->sleep_pin, "fp_sleep")) {
		err = -EBUSY;
		printk("[FP][%s] gpio request failed ! \n", __func__);
		goto fp_gpio_init_sleep_pin_failed;
	}

	/* ---no use in msm8953--- */
/*
	if (gpio_request(pdata->osvcc_pin, "fp_osvcc")) {
		err = -EBUSY;
		printk("[FP][%s] gpio request failed ! \n", __func__);
		goto fp_gpio_init_osvcc_pin_failed;
	}
*/
	/* ---no use in msm8953--- */

	/* config part */
	err = gpio_direction_output(pdata->sleep_pin, 1);
	if (err < 0) {
		printk("[FP][%s] gpio_direction_output SLEEP failed ! \n", __func__);
		err = -EBUSY;
		goto fp_gpio_config_sleep_pin_failed;
	}

	/* ---no use in msm8953--- */
/*
	err = gpio_direction_input(pdata->osvcc_pin);
	if (err < 0) {
		printk("[FP][%s] gpio_direction_output osvcc_pin failed ! \n", __func__);
		err = -EBUSY;
		goto fp_gpio_config_sleep_pin_failed;
	}

	gpio_set_value(pdata->osvcc_pin, 1);
*/
	/* ---no use in msm8953--- */

	return err;


fp_gpio_config_sleep_pin_failed:
	gpio_free(pdata->sleep_pin);
fp_gpio_init_sleep_pin_failed:
	gpio_free(pdata->drdy_pin);
fp_gpio_pwr_pin_failed:
		gpio_free(pdata->pwr_gpio);

	return err;
}
/* --- fp gpio init --- */


/* +++ fp power on/off +++ */
static int fp_power_on(struct fp_device_data *pdata, bool on)
{
	int rc = 0;

	if (!on)
		goto power_off;
	
    if (gpio_is_valid(pdata->pwr_gpio)) {
        gpio_set_value(pdata->pwr_gpio, 1);
    }
    msleep(10);
    printk("---- power on ok ----\n");

    return rc;
	
#if 0
	rc = regulator_enable(pdata->sovcc);
	if (rc) {
		printk("[FP][%s]Regulator sovcc enable failed rc=%d\n", __func__, rc);
		return rc;
	}
#endif
#if 0
	rc = regulator_enable(pdata->vcc);
	if (rc) {
		printk("[FP][%s]Regulator vcc enable failed rc=%d\n", __func__, rc);
		regulator_disable(pdata->sovcc);
	}

	return rc;
#endif
	return 0;

power_off:

	if (gpio_is_valid(pdata->pwr_gpio)) {
        gpio_set_value(pdata->pwr_gpio, 0);
    }
    msleep(10);
    printk("---- power off ok ----\n");
	
#if 0
	rc = regulator_disable(pdata->sovcc);
	if (rc) {
		printk("[FP][%s]Regulator sovcc disable failed rc=%d\n", __func__, rc);
		return rc;
	}
#endif

#if 0
	rc = regulator_disable(pdata->vcc);
	if (rc) {
		printk("[FP][%s]Regulator vcc disable failed rc=%d\n", __func__, rc);
		rc = regulator_enable(pdata->sovcc);
		if (rc) {
		printk("[FP][%s]Regulator sovcc disable failed rc=%d\n", __func__, rc);
		}
	}

	return rc;

#endif
	return 0;
}
/* --- fp power on/off --- */

/* +++ fp power init +++ */
static int fp_power_init(struct device *dev,
			struct fp_device_data *pdata, bool on)
{
	//int rc;

	if (!on)
		goto pwr_deinit;
	/* +++ pars regulator+++ */
#if 0
	pdata->sovcc = devm_regulator_get(dev, "sovcc");
	if (IS_ERR( pdata->sovcc)) {
		rc = PTR_ERR(pdata->sovcc);
		printk("Regulator get VFS sovcc  failed rc=%d\n", rc);
		return rc;
		}

	if (regulator_count_voltages(pdata->sovcc) > 0) {
		rc = regulator_set_voltage(pdata->sovcc, 3300000,
					   3300000);
		if (rc) {
			printk("Regulator set sovcc failed vdd rc=%d\n", rc);
			goto reg_sovcc_put;
		}
	}
#endif	

#if 0
	pdata->vcc = devm_regulator_get(dev, "vcc");
	if (IS_ERR( pdata->vcc)) {
		rc = PTR_ERR(pdata->vcc);
		printk("Regulator get vcc failed rc=%d\n", rc);
		goto reg_vdd_set_vtg;
		}

	if (regulator_count_voltages(pdata->vcc) > 0) {
		rc = regulator_set_voltage(pdata->vcc, 1800000,
					   1800000);
		if (rc) {
			printk("Regulator set_vcc failed vdd rc=%d\n", rc);
			goto reg_vcc_i2c_put;
		}
	}
#endif
	
	/* +++ pars regulator+++ */
	return 0;
#if 0 
reg_vcc_i2c_put:
	regulator_put(pdata->vcc);
reg_vdd_set_vtg:
	if (regulator_count_voltages(pdata->sovcc) > 0)
		regulator_set_voltage(pdata->sovcc, 0, 3300000);
//reg_sovcc_put:
//	regulator_put(pdata->sovcc);
//	return rc;
#endif

pwr_deinit:
//	if (regulator_count_voltages(pdata->sovcc) > 0)
//		regulator_set_voltage(pdata->sovcc, 0, 3300000);

//	regulator_put(pdata->sovcc);

#if 0	
	if (regulator_count_voltages(pdata->vcc) > 0)
		regulator_set_voltage(pdata->vcc, 0, 1800000);

	regulator_put(pdata->vcc);
#endif
	return 0;
}
/* --- fp power init --- */

/* vfsspidri pars dt data*/
static int fp_pars_dt(struct device *dev,
			struct fp_device_data *pdata)
{

	struct device_node *np = dev->of_node;
	
	/* +++reset, irq gpio info+++ */
	pdata->sleep_pin = of_get_named_gpio_flags(np, "qcom-spi-test,sleep-gpio",
				0, NULL);
	if (pdata->sleep_pin < 0)
		return pdata->sleep_pin;

	pdata->drdy_pin = of_get_named_gpio_flags(np, "qcom-spi-test,irq-gpio",
				0, NULL);
	if (pdata->drdy_pin < 0)
		return pdata->drdy_pin;

	pdata->pwr_gpio = of_get_named_gpio_flags(np,"qcom,gpio_pwr",0,NULL);
	if (pdata->pwr_gpio < 0)
		return pdata->pwr_gpio;

	/* ---reset, irq gpio info--- */

	/* ---no use in msm8953--- */
/*
	pdata->osvcc_pin = of_get_named_gpio_flags(np, "qcom-spi-test,osvcc-gpio",
				0, NULL);
	if (pdata->osvcc_pin < 0)
		return pdata->osvcc_pin;	
*/
	/* ---no use in msm8953--- */

	printk("[vfsspidry_pars_dt] sleep_pin = %d, drdy_pin = %d , pwr_gpio = %d\n", pdata->sleep_pin, pdata->drdy_pin,pdata->pwr_gpio);

	return 0;
}
/* vfsspidri pars dt data*/

/* --- common operate zone --- */

static int fp_sensor_probe(struct platform_device *pdev)
{
	int status = 0;
	struct fp_device_data *fp_device;
	struct device *dev = &pdev->dev;

	printk("[FP] Probe +++\n");
	fp_device = kzalloc(sizeof(*fp_device), GFP_KERNEL);
	if (fp_device == NULL) {
		printk("[FP] alloc Fingerprint data fail.\r\n");
		status = -1;
		goto fp_kmalloc_failed;
	}

	status = fp_pars_dt(&pdev->dev, fp_device);
	fp_device->module_vendor = 1;

	fp_device->irq_wakeup_flag = false;

	if (fp_power_init(&pdev->dev, fp_device, true) < 0){
		printk("[FP] opps fp_power_init ! \n");
		status = -3;
		goto fp_pars_dt_failed;
	}

	if (fp_power_on(fp_device, true) < 0) {
		printk("[FP] opps fp_power_on ! \n");
		status = -4;
		goto fp_pars_dt_failed;
	}
	
	if (fp_gpio_init(fp_device)) {
		printk("[FP] opps fp_gpio_init fail ! \n");
		status = -5;
		goto fp_pars_dt_failed;
	}

	dev_set_drvdata(&pdev->dev, fp_device);

	create_fingerprint_proc_file();

	wake_lock_init(&fp_device->wake_lock, WAKE_LOCK_SUSPEND, "FP_wake_lock");

	/* +++ switch FP vdndor +++ */
	spin_lock_init(&fp_device->vfs_spi_lock);
	mutex_init(&fp_device->buffer_mutex);
	mutex_init(&fp_device->kernel_lock);

	INIT_LIST_HEAD(&fp_device->device_entry);

	status = gpio_direction_input(fp_device->drdy_pin);
	if (status < 0) {
		pr_err("gpio_direction_input DRDY failed, status = %d \n", status);
		status = -8;
		goto fp_pars_dt_failed;
	}

	gpio_irq = gpio_to_irq(fp_device->drdy_pin);
	fp_device->isr_pin = gpio_irq;
	if (gpio_irq < 0) {
		pr_err("gpio_to_irq failed\n");
		status = -9;
		goto fp_pars_dt_failed;
	}

	if (request_irq(gpio_irq, vfsspi_irq, IRQF_TRIGGER_RISING,
			"vfsspi_irq", fp_device) < 0) {
		pr_err("request_irq failed \n");
		status = -10;
		goto fp_pars_dt_failed;
	}

	fp_device->is_drdy_irq_enabled = DRDY_IRQ_ENABLE;

	mutex_lock(&device_list_mutex);
	/* Create device node */
	/* register major number for character device */
	status = alloc_chrdev_region(&(fp_device->devt),
				     0, 1, VALIDITY_PART_NAME);
	if (status < 0) {
		pr_err("alloc_chrdev_region failed\n");
		goto fp_probe_alloc_chardev_failed;
	}

	cdev_init(&(fp_device->cdev), &vfsspi_fops);
	fp_device->cdev.owner = THIS_MODULE;
	status = cdev_add(&(fp_device->cdev), fp_device->devt, 1);
	if (status < 0) {
		pr_err("cdev_add failed\n");
		unregister_chrdev_region(fp_device->devt, 1);
		goto fp_probe_cdev_add_failed;
	}

	vfsspi_device_class = class_create(THIS_MODULE, "validity_fingerprint");

	if (IS_ERR(vfsspi_device_class)) {
		pr_err("vfsspi_init: class_create() is failed - unregister chrdev.\n");
		cdev_del(&(fp_device->cdev));
		unregister_chrdev_region(fp_device->devt, 1);
		status = PTR_ERR(vfsspi_device_class);
		goto vfsspi_probe_class_create_failed;
	}

	dev = device_create(vfsspi_device_class, &pdev->dev,
			    fp_device->devt, fp_device, "vfsspi");
	status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
	if (status == 0)
		list_add(&fp_device->device_entry, &device_list);
	mutex_unlock(&device_list_mutex);

	if (status != 0)
		goto vfsspi_probe_failed;

//			enable_irq_wake(gpio_irq);

#if 0
			status = sysfs_create_group(&spi->dev.kobj, &vfsspi_attribute_group);
			if (0 != status) {
				printk("[FP][FP_ERR] %s() - ERROR: sysfs_create_group() failed.error code: %d\n", __FUNCTION__, status);
				sysfs_remove_group(&spi->dev.kobj, &vfsspi_attribute_group);
				return -EIO;
			} else {
				printk("[FP][FP_ERR] %s() - sysfs_create_group() succeeded. \n",  __FUNCTION__);
			}
#endif

	/* --- switch FP vdndor --- */
	printk("[FP] change owner ship end! \n");
	
	return 0;


vfsspi_probe_failed:
vfsspi_probe_class_create_failed:
fp_probe_cdev_add_failed:
fp_probe_alloc_chardev_failed:
fp_pars_dt_failed:
	kfree(fp_device);
fp_kmalloc_failed:
	printk("[FP] Probe failed wieh error code %d ! \n", status);
	return status;
}


static int fp_sensor_remove(struct platform_device *pdev)
{
	return 0;
}

static struct of_device_id fp_match_table[] = {
	{ .compatible = "asus,fingerprint",},
	{ },
};

static struct platform_driver fp_sensor_driver = {
	.driver = {
		.name = "fphandle",
		.owner = THIS_MODULE,
		.of_match_table = fp_match_table,
	},
	.probe         	= fp_sensor_probe,
	.remove			= fp_sensor_remove,
	.suspend		= fp_sensor_suspend,
	.resume			= fp_sensor_resume,
};


static int __init fp_sensor_init(void)
{	
	int err = 0;
	if(asus_fp_id != SYNAPTICS)
		return 0;
	
	printk("[FP] Driver INIT +++\n");

	err = platform_driver_register(&fp_sensor_driver);
	if (err != 0) {
		printk("[FP] platform_driver_register fail, Error : %d\n", err);
	}
	
	printk("[FP] Driver INIT ---\n");

	return err;
}

static void __exit fp_sensor_exit(void)
{
	printk("[FP] Driver EXIT +++\n");
	
	platform_driver_unregister(&fp_sensor_driver);	
	
	printk("[FP] Driver EXIT ---\n");
}


module_init(fp_sensor_init);
module_exit(fp_sensor_exit);

MODULE_AUTHOR("jacob_kung <jacob_kung@asus.com>");
MODULE_DESCRIPTION("asus fingerprint handle");
MODULE_LICENSE("GPL v2");
