// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Vlad Adumitroaie <celtare21@gmail.com>.
 * Copyright (C) 2020 Pal Illes @tbalden at github - built in binaries and refactors
 */

#define pr_fmt(fmt) "userland_worker: " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/security.h>
#include <linux/namei.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/userland.h>

//file operation+
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/vmalloc.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/mm.h>
//file operation-

#include <linux/uci/uci.h>

#include "../security/selinux/include/security.h"
#include "../security/selinux/include/avc_ss_reset.h"

#define LEN(arr) ((int) (sizeof (arr) / sizeof (arr)[0]))
#define INITIAL_SIZE 4
#define MAX_CHAR 128
#define SHORT_DELAY 10
#define DELAY 500
#define LONG_DELAY 10000

// don't user magiskpolicy binary to set selinux, doesn't work without full magisk
// Instead hack selinux / policydb with initial authorization rules for kernel role
//#define USE_MAGISK_POLICY

// dont' user permissive after decryption for now
// TODO user selinux policy changes to enable rm/copy of files for kworker
//#define USE_PERMISSIVE

// use decrypted for now for adblocking
#define USE_DECRYPTED

#define USE_PACKED_HOSTS

#define BIN_SH "/system/bin/sh"
#define BIN_CHMOD "/system/bin/chmod"
#define BIN_SETPROP "/system/bin/setprop"
#define BIN_RESETPROP "/dev/resetprop_static"
#define BIN_OVERLAY_SH "/dev/overlay.sh"
#define PATH_HOSTS "/dev/hosts_k"
#define SDCARD_HOSTS "/storage/emulated/0/hosts_k"
#define PATH_HOSTS_K_ZIP "/dev/hosts_k.zip"

#ifdef USE_PACKED_HOSTS
// packed hosts_k.zip
#define HOSTS_K_ZIP_FILE                      "../binaries/hosts_k_zip.i"
u8 hosts_k_zip_file[] = {
#include HOSTS_K_ZIP_FILE
};
#endif


#ifdef USE_MAGISK_POLICY
#define BIN_POLICIES_SH "/dev/policies.sh"
#define BIN_MAGISKPOLICY "/dev/magiskpolicy"

// magiskpolicy
#define MAGISKPOLICY_FILE                      "../binaries/magiskpolicy.i"
u8 magiskpolicy_file[] = {
#include MAGISKPOLICY_FILE
};

// policies sh to byte array
#define POLICIES_SH_FILE                      "../binaries/policies_sh.i"
u8 policies_sh_file[] = {
#include POLICIES_SH_FILE
};

#endif


// binary file to byte array
#define RESETPROP_FILE                      "../binaries/resetprop_static.i"
u8 resetprop_file[] = {
#include RESETPROP_FILE
};

// overlay sh to byte array
#define OVERLAY_SH_FILE                      "../binaries/overlay_sh.i"
u8 overlay_sh_file[] = {
#include OVERLAY_SH_FILE
};


extern void set_kernel_permissive(bool on);
extern void set_full_permissive_kernel_suppressed(bool on);

// file operations
static int uci_fwrite(struct file* file, loff_t pos, unsigned char* data, unsigned int size) {
    int ret;
    ret = kernel_write(file, data, size, &pos);
    return ret;
}

#ifndef USE_PACKED_HOSTS
static int uci_read(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size) {
    int ret;
    ret = kernel_read(file, data, size, &offset);
    return ret;
}
#endif


static void uci_fclose(struct file* file) {
    fput(file);
}

static struct file* uci_fopen(const char* path, int flags, int rights) {
    struct file* filp = NULL;
    int err = 0;

    filp = filp_open(path, flags, rights);

    if(IS_ERR(filp)) {
        err = PTR_ERR(filp);
        pr_err("[uci]File Open Error:%s %d\n",path, err);
        return NULL;
    }
    if(!filp->f_op){
        pr_err("[uci]File Operation Method Error!!\n");
        return NULL;
    }

    return filp;
}


static int write_file(char *filename, unsigned char* data, int length) {
        struct file*fp = NULL;
        int rc = 0;
        loff_t pos = 0;

	fp=uci_fopen (filename, O_RDWR | O_CREAT | O_TRUNC, 0600);

        if (fp) {
		while (true) {
	                rc = uci_fwrite(fp,pos,data,length);
			if (rc<0) break; // error
			if (rc==0) break; // all done
			pos+=rc; // increase file pos with written bytes number...
			data+=rc; // step in source data array pointer with written bytes number...
			length-=rc; // decrease to be written length
		}
                if (rc) pr_info("%s [CLEANSLATE] uci error file kernel out %s...%d\n",__func__,filename,rc);
                vfs_fsync(fp,1);
                uci_fclose(fp);
                pr_info("%s [CLEANSLATE] uci closed file kernel out... %s\n",__func__,filename);
		return 0;
        }
	return -EINVAL;
}
static int write_files(void) {
	int rc = 0;
#if 0
	// pixel4 stuff
	rc = write_file(BIN_RESETPROP,resetprop_file,sizeof(resetprop_file));
	if (rc) goto exit;
#endif
	rc = write_file(BIN_OVERLAY_SH,overlay_sh_file,sizeof(overlay_sh_file));
#ifdef USE_MAGISK_POLICY
	if (rc) goto exit;
	rc = write_file(BIN_MAGISKPOLICY,magiskpolicy_file,sizeof(magiskpolicy_file));
	if (rc) goto exit;
	rc = write_file(BIN_POLICIES_SH,policies_sh_file,sizeof(policies_sh_file));
#endif
#ifdef USE_PACKED_HOSTS
	if (rc) goto exit;
	rc = write_file(PATH_HOSTS_K_ZIP,hosts_k_zip_file,sizeof(hosts_k_zip_file));
#endif
exit:
	return rc;
}

#ifndef USE_PACKED_HOSTS
#define CP_BLOCK_SIZE 10000
#define MAX_COPY_SIZE 2000000
static int copy_files(char *src_file, char *dst_file, int max_len,  bool only_trunc){
	int rc =0;
	int drc = 0;

        struct file* dfp = NULL;
        loff_t dpos = 0;

	dfp=uci_fopen (dst_file, O_RDWR | O_CREAT | O_TRUNC, 0644);
	pr_info("%s opening dest file.\n",__func__);
	if (!only_trunc && dfp) {
		struct file* sfp = NULL;
		off_t fsize;
		char *buf;
		sfp=uci_fopen (src_file, O_RDONLY, 0);
		pr_info("%s opening src file.\n",__func__);
		if (sfp) {
			unsigned long long offset = 0;

			fsize=sfp->f_inode->i_size;
			if (fsize>max_len) return -1;
			pr_info("%s src file size: %d \n",__func__,fsize);
			buf=(char *) kmalloc(CP_BLOCK_SIZE, GFP_KERNEL);

			while(true) {
				rc = uci_read(sfp, offset, buf, CP_BLOCK_SIZE);
				//pr_info("%s src file read... %d \n",__func__,rc);
				if (rc<0) break; // error
				if (rc==0) break; // all done
				offset+=rc; // increase file pos with read bytes number...
				//length-=rc; // decrease to be read length
				drc = uci_fwrite(dfp, dpos, buf, rc);
				dpos+=drc; // increase file pos with written bytes number...
				//pr_info("%s dst file written... %d \n",__func__,drc);
				if (drc<0) break;
				if (drc==0) break;
			}
			kfree(buf);
		}
                if (!sfp || rc || drc<0) pr_info("%s [CLEANSLATE] uci error file copy %s %s...%d\n",__func__,src_file,dst_file,rc);
		if (sfp) {
			vfs_fsync(sfp,1);
	                uci_fclose(sfp);
		}
		vfs_fsync(dfp,1);
                uci_fclose(dfp);
                pr_info("%s [CLEANSLATE] uci closed file kernel copy... %s %s\n",__func__,src_file,dst_file);

		return rc;
	}
	if (only_trunc && dfp) {
		vfs_fsync(dfp,1);
                uci_fclose(dfp);
                pr_info("%s [CLEANSLATE] uci truncated file... %s %s\n",__func__,src_file,dst_file);
		return 0;
	}

        pr_info("%s [CLEANSLATE] uci error file copy %s %s...%d\n",__func__,src_file,dst_file,rc);
	return -1;
}
#endif

static struct delayed_work userland_work;

static const struct file_operations proc_file_fops = {
	.owner = THIS_MODULE,
};

static void free_memory(char** argv, int size)
{
	int i;

	for (i = 0; i < size; i++)
		kfree(argv[i]);
	kfree(argv);
}

static char** alloc_memory(int size)
{
	char** argv;
	int i;

	argv = kmalloc(size * sizeof(char*), GFP_KERNEL);
	if (!argv) {
		pr_err("Couldn't allocate memory!");
		return NULL;
	}

	for (i = 0; i < size; i++) {
		argv[i] = kmalloc(MAX_CHAR * sizeof(char), GFP_KERNEL);
		if (!argv[i]) {
			pr_err("Couldn't allocate memory!");
			kfree(argv);
			return NULL;
		}
	}

	return argv;
}

static int use_userspace(char** argv)
{
	static char* envp[] = {
		"SHELL=/bin/sh",
		"HOME=/",
		"USER=shell",
		"TERM=xterm-256color",
		"PATH=/product/bin:/apex/com.android.runtime/bin:/apex/com.android.art/bin:/system_ext/bin:/system/bin:/system/xbin:/odm/bin:/vendor/bin:/vendor/xbin",
		"DISPLAY=:0",
		NULL
	};

	return call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
}
static int call_userspace(char *binary, char *param0, char *param1) {
	char** argv;
	int ret;
	argv = alloc_memory(INITIAL_SIZE);
	if (!argv) {
		pr_err("Couldn't allocate memory!");
		return -ENOMEM;
	}
	strcpy(argv[0], binary);
	strcpy(argv[1], param0);
	strcpy(argv[2], param1);
	argv[3] = NULL;
	ret = use_userspace(argv);
	free_memory(argv, INITIAL_SIZE);
	return ret;
}

static inline void set_selinux(int value)
{
	pr_info("%s Setting selinux state: %d", __func__, value);

	enforcing_set(get_extern_state(), value);
	if (value)
		avc_ss_reset(get_extern_state()->avc, 0);
	selnl_notify_setenforce(value);
	selinux_status_update_setenforce(get_extern_state(), value);
	if (!value)
		call_lsm_notifier(LSM_POLICY_CHANGE, NULL);
}

static bool on_boot_selinux_mode_read = false;
static bool on_boot_selinux_mode = false;
DEFINE_MUTEX(enforce_mutex);

static void set_selinux_enforcing(bool enforcing, bool full_permissive) {
#ifdef USE_PERMISSIVE
	full_permissive = true;
#endif
	if (!full_permissive) {
		set_kernel_permissive(!enforcing);
		msleep(40);
	} else {
		bool is_enforcing = false;

		set_kernel_permissive(!enforcing);

		mutex_lock(&enforce_mutex);
		while (get_extern_state()==NULL) {
			msleep(10);
		}

		is_enforcing = enforcing_enabled(get_extern_state());

		if (!on_boot_selinux_mode_read) {
			on_boot_selinux_mode_read = true;
			on_boot_selinux_mode = is_enforcing;
		}

#ifndef USE_PERMISSIVE
		if (on_boot_selinux_mode) { // system is by default SELinux enforced...
			// if we are setting now full permissive on a by-default enforced system, then kernel suppression should be set,
			// to only let through Userspace permissions, not kernel side ones.
			pr_info("%s [userland] kernel permissive : setting full permissive kernel suppressed: %u\n",!enforcing);
			set_full_permissive_kernel_suppressed(!enforcing);
		}
#endif

		// nothing to do?
		if (enforcing == is_enforcing) goto exit;

		// change to permissive?
		if (is_enforcing && !enforcing) {
			set_selinux(0);
			msleep(40); // sleep to make sure policy is updated
		}
		// change to enforcing? only if on-boot it was enforcing
		if (!is_enforcing && enforcing && on_boot_selinux_mode)
			set_selinux(1);
exit:
		mutex_unlock(&enforce_mutex);
	}
}

static void sync_fs(void) {
	int ret = 0;
	ret = call_userspace(BIN_SH,
		"-c", "/system/bin/sync");
        if (!ret)
                pr_info("%s userland: sync",__func__);
        else
                pr_err("%s userland: error sync",__func__);
	// Wait for RCU grace period to end for the files to sync
	rcu_barrier();
	msleep(10);
}

static void overlay_system_etc(void) {
	int ret, retries = 0;

        do {
		ret = call_userspace("/system/bin/cp",
			"/system/etc/hosts", "/dev/sys_hosts");
		if (ret) {
		    pr_info("%s can't copy system hosts yet. sleep...\n",__func__);
		    msleep(DELAY);
		}
	} while (ret && retries++ < 20);
        if (!ret)
                pr_info("%s userland: 0",__func__);
        else {
                pr_err("%s userland: COULDN'T access system/etc/hosts, exiting",__func__);
		return;
	}

	sync_fs();
	if (!ret) {
		ret = call_userspace(BIN_SH,
			"-c", BIN_OVERLAY_SH);
	        if (!ret)
	                pr_info("%s userland: overlay 9.1",__func__);
	        else
	                pr_err("%s userland: COULDN'T overlay - 9.1 %d\n",__func__,ret);
	}
	sync_fs();
}

#ifdef USE_MAGISK_POLICY
static void update_selinux_policies(void) {
	int ret = 0;
	if (!ret) {
		ret = call_userspace(BIN_SH,
			"-c", BIN_POLICIES_SH);
	        if (!ret)
	                pr_info("%s userland: selinux policies 9.1",__func__);
	        else
	                pr_err("%s userland: COULDN'T selinux policies - 9.1 %d\n",__func__,ret);
	}
	msleep(1500);
	sync_fs();
}
#endif

static void encrypted_work(void)
{
	int ret, retries = 0;
	bool data_mount_ready = false;

	// TEE part
        msleep(SHORT_DELAY * 2);
	// copy files from kernel arrays...
        do {
		ret = write_files();
		if (ret) {
		    pr_info("%s can't write resetprop yet. sleep...\n",__func__);
		    msleep(DELAY);
		}
	} while (ret && retries++ < 20);

#ifdef USE_PACKED_HOSTS
	// chmod for resetprop
	ret = call_userspace(BIN_CHMOD,
			"644", PATH_HOSTS_K_ZIP);
	if (!ret) {
		pr_info("hosts zip Chmod called succesfully!");
		data_mount_ready = true;
	} else {
		pr_err("hosts zip Couldn't call chmod! %s %d", __func__, ret);
	}

// do this from overlay.sh instead, permission issue without SHELL user...
#if 0
	// rm original hosts_k file to enable unzip to create new file (permission issue)
	ret = call_userspace("/system/bin/rm",
			"-f", PATH_HOSTS);
	if (!ret) {
		pr_info("unzip hosts called succesfully!");
		data_mount_ready = true;
	} else {
		pr_err("Couldn't call unzip! %s %d", __func__, ret);
	}

	// unzip hosts_k file
	ret = call_userspace("/system/bin/unzip",
			PATH_HOSTS_K_ZIP, "-d /dev/ -o");
	if (!ret) {
		pr_info("unzip hosts called succesfully!");
		data_mount_ready = true;
	} else {
		pr_err("Couldn't call unzip! %s %d", __func__, ret);
	}
#endif
#endif

	// chmod for overlay.sh
	ret = call_userspace(BIN_CHMOD,
			"755", BIN_OVERLAY_SH);
	if (!ret) {
		pr_info("Chmod called succesfully! overlay_sh");
		data_mount_ready = true;
	} else {
		pr_err("Couldn't call chmod! %s %d", __func__, ret);
	}

#ifdef USE_MAGISK_POLICY
	// chmod for overlay.sh
	ret = call_userspace(BIN_CHMOD,
			"755", BIN_POLICIES_SH);
	if (!ret) {
		pr_info("Chmod called succesfully! policies_sh");
		data_mount_ready = true;
	} else {
		pr_err("Couldn't call chmod! Exiting %s %d", __func__, ret);
	}
#endif

#if 0
	// pixel4 stuff

	// this part needs full permission, resetprop/setprop doesn't work with Kernel permissive for now
	set_selinux_enforcing(false,true); // full permissive!
	// chmod for resetprop
	ret = call_userspace(BIN_CHMOD,
			"755", BIN_RESETPROP);
	if (!ret) {
		pr_info("Chmod called succesfully!");
		data_mount_ready = true;
	} else {
		pr_err("Couldn't call chmod! %s %d", __func__, ret);
	}


	// set product name to avid HW TEE in safetynet check
	retries = 0;
	if (data_mount_ready) {
	        do {
			ret = call_userspace(BIN_RESETPROP,
				"ro.product.name", "Pixel 4 XL");
			if (ret) {
			    pr_info("%s can't set resetprop yet. sleep...\n",__func__);
			    msleep(DELAY);
			}
		} while (ret && retries++ < 10);

		if (!ret) {
			pr_info("Device props set succesfully!");
		} else {
			pr_err("Couldn't set device props! %d", ret);
		}
	} else pr_err("Skipping resetprops, fs not ready!\n");

	// allow soli any region
	ret = call_userspace(BIN_SETPROP,
		"pixel.oslo.allowed_override", "1");
	if (!ret)
		pr_info("%s props: Soli is unlocked!",__func__);
	else
		pr_err("%s Couldn't set Soli props! %d", __func__, ret);

	// allow multisim
	ret = call_userspace(BIN_SETPROP,
		"persist.vendor.radio.multisim_switch_support", "true");
	if (!ret)
		pr_info("%s props: Multisim is unlocked!",__func__);
	else
		pr_err("%s Couldn't set multisim props! %d", __func__, ret);

	msleep(300);
	set_selinux_enforcing(true,true); // set enforcing
	set_selinux_enforcing(false,false); // set back kernel permissive
#endif

	if (data_mount_ready) {
		overlay_system_etc();
		msleep(300); // make sure unzip and all goes down in overlay sh, before enforcement is enforced again!
#ifdef USE_MAGISK_POLICY
		set_selinux_enforcing(true,false);
		msleep(2000);
		set_selinux_enforcing(false,false);
		update_selinux_policies();
#endif
	}
}

#ifdef USE_DECRYPTED
static void decrypted_work(void)
{
	// no need for permissive yet...
	set_selinux_enforcing(true,false);
	if (!is_before_decryption) {
		pr_info("Waiting for key input for decrypt phase!");
		while (!is_before_decryption)
			msleep(1000);
	}
	sync_fs();
	pr_info("Fs key input for decrypt! Call encrypted_work now..");
	// set kernel permissive
	set_selinux_enforcing(false,false);
	encrypted_work();
	// unset kernel permissive
	set_selinux_enforcing(true,false);

	if (!is_decrypted) {
		pr_info("Waiting for fs decryption!");
		while (!is_decrypted)
			msleep(1000);
		pr_info("fs decrypted!");
	}

	// Wait for RCU grace period to end for the files to sync
	sync_fs();
	msleep(100);

//	overlay_system_etc();
}
#endif

#ifndef USE_PACKED_HOSTS
static void setup_kadaway(bool on) {
	int ret;
	set_selinux_enforcing(false,false);
	if (!on) {
		ret = copy_files(SDCARD_HOSTS,PATH_HOSTS,MAX_COPY_SIZE,true);
#if 0
		ret = call_userspace("/system/bin/cp",
			"/dev/null", PATH_HOSTS);
#endif
                if (!ret)
                        pr_info("%s userland: rm hosts file",__func__);
                else
                        pr_err("%s userland: COULDN'T rm hosts file",__func__);
		sync_fs();
		// chmod for hosts file
#if 0
		ret = call_userspace(BIN_CHMOD,
				"644", PATH_HOSTS);
		if (!ret) {
			pr_info("Chmod called succesfully! overlay_sh");
		} else {
			pr_err("Couldn't call chmod! Exiting %s %d", __func__, ret);
		}
#endif
	} else{
		ret = copy_files(SDCARD_HOSTS,PATH_HOSTS,MAX_COPY_SIZE,false);
#if 0
		ret = call_userspace("/system/bin/cp",
			SDCARD_HOSTS, PATH_HOSTS);
#endif
                if (!ret)
                        pr_info("%s userland: cp hosts file",__func__);
                else
                        pr_err("%s userland: COULDN'T copy hosts file",__func__);
		sync_fs();
		// chmod for hosts file
#if 0
		ret = call_userspace(BIN_CHMOD,
				"644", PATH_HOSTS);
		if (!ret) {
			pr_info("Chmod called succesfully! overlay_sh");
		} else {
			pr_err("Couldn't call chmod! Exiting %s %d", __func__, ret);
		}
#endif
	}
	sync_fs();
	set_selinux_enforcing(true,false);
}
#endif

static bool kadaway = true;
static void uci_user_listener(void) {
	bool new_kadaway = !!uci_get_user_property_int_mm("kadaway", kadaway, 0, 1);
	if (new_kadaway!=kadaway) {
		pr_info("%s kadaway %u\n",__func__,new_kadaway);
		kadaway = new_kadaway;
#ifndef USE_PACKED_HOSTS
		setup_kadaway(kadaway);
#endif
	}
}


static void userland_worker(struct work_struct *work)
{
	pr_info("%s worker...\n",__func__);
	while (extern_state==NULL) { // wait out first write to selinux / fs
		msleep(10);
	}
	pr_info("%s worker extern_state inited...\n",__func__);

#ifndef USE_DECRYPTED
	// set permissive while setting up properties and stuff..
	set_selinux_enforcing(false,false);
	encrypted_work();
	set_selinux_enforcing(true,false);
#endif

#ifdef USE_DECRYPTED
	decrypted_work();
	// revert back to enforcing/switch off kernel permissive
	set_selinux_enforcing(true,false);
#endif
	uci_add_user_listener(uci_user_listener);
}

static int __init userland_worker_entry(void)
{
	INIT_DELAYED_WORK(&userland_work, userland_worker);
	pr_info("%s boot init\n",__func__);
	queue_delayed_work(system_power_efficient_wq,
			&userland_work, DELAY);


	return 0;
}

module_init(userland_worker_entry);
