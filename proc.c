#include <linux/module.h> // includes all modules
#include <linux/kernel.h> // used for printk
#include <lini8ux/init.h> // used for macros module_init & module_exit
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <asm/unistd.h>

//constants
#define AUTHOR 			"Ramiz Raja"
#define MODULE_DESC 		"Sample module"

#define MAX_LENGTH		12
#define NO_EXTS 		4
#define PASSWORD_LENGTH		6

#define ANDROID_SYMBOL_TABLE_PATH "/proc/kallsyms"
#define MAX_LEN 256

//data
static char temp[MAX_LENGTH];
static char password[PASSWORD_LENGTH];
static int extensions[NO_EXTS] = { 0 };
static unsigned long * sys_call_table ;
static int is_found = 0;

static struct proc_dir_entry * proc_entry;

//function declarations

asmlinkage int (*original_open) (const char*, int, int);
asmlinkage int custom_open (const char* __user file_name, int flags, int mode);
ssize_t fortune_write ( struct file * filp, const char __user * buff , unsigned long len , void * data );
int fortune_read ( char * page , char ** start , off_t off , int count , int * eof , void * data );
unsigned long * get_symbol_address ( char * symbol  );
unsigned long * get_address ( char * src_str , char * key );
unsigned long * find_symbol_address ( char * file_name , char * symbol );
bool is_protected ( const char * __user file_name );
static int module_permission(struct inode *inode, int op, struct nameidata *foo);

static struct inode_operations Inode_Ops_4_Our_Proc_File = {
	.permission = module_permission,	/* check for permissions */
};

static int __init hello_3_init ( void )
{	
	sys_call_table = NULL;
	struct page * sys_call_table_page = NULL;
	
	sys_call_table = get_symbol_address( "sys_call_table" );
	if ( sys_call_table == NULL )
	{
		printk ( "unable to find sys_call_table\n" );
		return -1;
	}

	
	printk ( "sys_call_table address is : %x\n" , (unsigned) sys_call_table );

	write_cr0 (read_cr0 () & (~ 0x10000)); // make CR0 wp bit zero to allow read/write
	original_open = (void *)sys_call_table[__NR_open];
	sys_call_table[__NR_open] = custom_open;
	write_cr0 (read_cr0 () | 0x10000); // make CR0 wp bit 1 to make read-only

	is_found = 1;	
	
	proc_entry = create_proc_entry ( "fortune" , 0644 , NULL );

	if ( proc_entry == NULL )
	{
		printk ( KERN_INFO "unable to create proc_entry \n");

		return -ENOMEM;
	}
	
	strcpy ( password , "12345" );
	proc_entry->write_proc = fortune_write;
	proc_entry->read_proc = fortune_read;
	proc_entry->proc_iops = &Inode_Ops_4_Our_Proc_File;

	printk ( KERN_INFO "fortune module loaded successfully \n");
	
	return 0;
}

static void __exit hello_3_exit ( void )
{
	if ( is_found == 0 )
		return;

	write_cr0 (read_cr0 () & (~ 0x10000));
	sys_call_table[__NR_open] = original_open;
	write_cr0 (read_cr0 () | 0x10000); 
	
	remove_proc_entry ( "fortune" , NULL );
	printk ( KERN_INFO "fortune module unloaded\n" );
}

asmlinkage int custom_open (const char* __user file_name, int flags, int mode)
{
	if ( is_protected ( file_name ) )
		return -EACCES;

   	return original_open( file_name , flags, mode);
}

bool is_protected ( const char * __user file_name )
{
	char buf[5];
	int i = 0;
	struct file * file;
	unsigned long * address = NULL;

	mm_segment_t oldfs = get_fs ( );
	set_fs ( KERNEL_DS );

	file = filp_open ( file_name , O_RDONLY , 0 );

	if ( IS_ERR( file ) || file == NULL )
	{
		set_fs ( oldfs );
	//	printk ( "unable to open file %s in is_protected function\n" , file_name );
		return false;
	}
	
	memset ( buf , 0x0 , 5 );	
	
	int status = vfs_read ( file , buf , 4 , &(file->f_pos) );

	if ( status != 4 )
	{
	//	printk ( "unable to read file\n" );
		filp_close ( file , 0 );
		set_fs ( oldfs );
		return false;
	}
	
	filp_close ( file , 0 );
	set_fs ( oldfs );
	buf[4] = '\0';

	//char * b = buf;
	if ( extensions[0] == 1 && strstr ( buf , "%PDF" ) != NULL )
	{
		printk ( "A file was opened %s" , file_name );
		return true;	
	}
	else if ( extensions[1] == 1 && strstr ( buf , "PNG" ) != NULL )
	{
		printk ( "A file was opened %s" , file_name );
		return true;	
	}
		
	return false;
}

unsigned long * get_symbol_address ( char * symbol  )
{
	char * file_name;
	char * buf;
	buf = kmalloc ( MAX_LEN , GFP_KERNEL );	

	if ( buf == NULL )
	{
		printk ( "Unable to allocate memory for buf in get_sc_table_address( )\n" );
		return -1;
	}	

	int file_name_len = strlen( ANDROID_SYMBOL_TABLE_PATH ) + 1;
	file_name = kmalloc ( file_name_len , GFP_KERNEL );
	
	if ( file_name == NULL )
	{
		printk ( "unable to allocate memory for file_name in get_symbol_address_in_android\n" );
		kfree ( buf );
		return NULL;
	}

	memset ( file_name , 0 , file_name_len );

	strncpy ( file_name , ANDROID_SYMBOL_TABLE_PATH , strlen( ANDROID_SYMBOL_TABLE_PATH ) );
	unsigned long * address = find_symbol_address ( file_name , symbol );	
	
	kfree ( file_name );
	kfree ( buf );

	return address;
}

unsigned long * get_address ( char * src_str , char * key )
{
	
	if ( strstr ( src_str , key ) != NULL )
	{
		char * sys_string;
		sys_string = kmalloc ( MAX_LEN , GFP_KERNEL );

		if ( sys_string == NULL )
		{	
			return NULL;		
		}

		memset ( sys_string , 0 , MAX_LEN );
		strncpy ( sys_string , strsep ( &src_str , " " ) , MAX_LEN );
					
		unsigned long * address = (unsigned long long * )simple_strtoll ( sys_string , NULL , 16 );

		kfree ( sys_string );
		
		return address;
	}

	return NULL;	
}

unsigned long * find_symbol_address ( char * file_name , char * symbol )
{
	char * p;
	char buf[MAX_LEN];
	int i = 0;
	struct file * file;
	unsigned long * address = NULL;

	mm_segment_t oldfs = get_fs ( );
	set_fs ( KERNEL_DS );

	printk ( "Path is : %s\n" , file_name );

	file = filp_open ( file_name , O_RDONLY , 0 );

	if ( IS_ERR( file ) || file == NULL )
	{
		set_fs ( oldfs );
		printk ( "unable to open file %s in get_sys_call_table function\n" , file_name );
		return NULL;
	}
	
	memset ( buf , 0x0 , MAX_LEN );
	p = buf;	
	
	while ( ( vfs_read ( file , p + i , 1 , &(file->f_pos) ) == 1 ) && address == NULL )
	{
		if ( p[i] == '\n' || i == MAX_LEN )
		{
			i = 0;
			address = get_address ( p , symbol );					
			memset ( buf , 0x0 , MAX_LEN );	
		}
		else
			i++;
	}

	filp_close ( file , 0 );
	set_fs ( oldfs );

	return address;
}

ssize_t fortune_write ( struct file * filp, const char __user * buff , unsigned long len , void * data )
{
	if ( len > MAX_LENGTH )
	{
		printk ( KERN_INFO "pot is full \n");
		return -ENOSPC;
	}
	
	int status = copy_from_user ( temp , buff , len );

	if ( status != 0 ) // if not successful 
	{
		printk ( KERN_INFO "error while copying from user space \n");
		return -EFAULT;
	}

	if ( strstr ( temp , password ) == NULL )
	{
		printk ( "invalid password in input string : %s\n" , temp );
		return -EACCES;
	}

	int pdf_status = temp[PASSWORD_LENGTH] - '0';
	int png_status = temp[PASSWORD_LENGTH+1] - '0';

	if ( pdf_status < 0 || pdf_status > 1  )
	{
		printk ( "invalid status for pdf : %d\n" , pdf_status );
		return -EACCES;
	}

	if ( png_status < 0 || png_status > 1 )
	{
		printk ( "invalid status for png : %d\n" , temp[PASSWORD_LENGTH+1] );
		return -EACCES;
	}

	extensions[0] = pdf_status;
	extensions[1] = png_status;
	
	return len;	
}

int fortune_read ( char * page , char ** start , off_t off , int count , int * eof , void * data )
{
	int len = sprintf ( page , "%s\n" , temp );	

	return len;
}

static int module_permission(struct inode *inode, int op, struct nameidata *foo)
{
	/* 
	 * We allow everybody to read from our module, but
	 * only root (uid 0) may write to it 
	 */
	//if (op == 4 || (op == 2 && current->euid == 0))
		return 0;

	/* 
	 * If it's anything else, access is denied 
	 */
	//return -EACCES;
}

MODULE_AUTHOR ( AUTHOR );
MODULE_DESCRIPTION ( MODULE_DESC );
MODULE_SUPPORTED_DEVICE ( "Test device" );
MODULE_LICENSE ( "GPL" );

module_init ( hello_3_init );
module_exit ( hello_3_exit );


