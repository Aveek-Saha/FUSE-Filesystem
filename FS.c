#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

// gcc FS.c -o FS `pkg-config fuse --cflags --libs`
// ./ FS - f Desktop / OS / mountpoint4

#define block_size 1024

typedef struct superblock {
	char datablocks[block_size*100];		//total number of data blocks
	char data_bitmap[105];      			//array of data block numbers that are available
	char inode_bitmap[105];   				//array of inode numbers that are available
} superblock;

typedef struct inode {
	int datablocks[16];            //data block number that the inode points to
	int number;
	int blocks;                    //==number of blocks the particular inode points to
	//int link;                    //==number of links
	int size;                      //==size of file/directory
} inode;

typedef struct filetype {
	int valid;
	char test[10];
	char path[100];
	char name[100];           //name
	inode *inum;              //inode number
	struct filetype ** children;
	int num_children;
	int num_links;
	struct filetype * parent;
	char type[20];                  //==file extension
	mode_t permissions;		        // Permissions
	uid_t user_id;		            // userid
	gid_t group_id;		            // groupid
	time_t a_time;                  // Access time
	time_t m_time;                  // Modified time
	time_t c_time;                  // Status change time
	time_t b_time;                  // Creation time
	off_t size;                     // Size of the node

	int datablocks[16];
	int number;
	int blocks;

} filetype;



superblock spblock;

void initialize_superblock(){

	memset(spblock.data_bitmap, '0', 100*sizeof(char));
	memset(spblock.inode_bitmap, '0', 100*sizeof(char));
}

filetype * root;

filetype file_array[50];

void tree_to_array(filetype * queue, int * front, int * rear, int * index){

	if(rear < front)
		return;
	if(*index > 30)
		return;


	filetype curr_node = queue[*front];
	*front += 1;
	file_array[*index] = curr_node;
	*index += 1;

	if(*index < 6){

		if(curr_node.valid){
			int n = 0;
			int i;
			for(i = 0; i < curr_node.num_children; i++){
				if(*rear < *front)
					*rear = *front;
				queue[*rear] = *(curr_node.children[i]);
				*rear += 1;
			}
			while(i<5){
				filetype waste_node;
				waste_node.valid = 0;
				queue[*rear] = waste_node;
				*rear += 1;
				i++;
			}
		}
		else{
			int i = 0;
			while(i<5){
				filetype waste_node;
				waste_node.valid = 0;
				queue[*rear] = waste_node;
				*rear += 1;
				i++;
			}
		}
	}

	tree_to_array(queue, front, rear, index);

}


int save_contents(){
	printf("SAVING\n");
	filetype * queue = malloc(sizeof(filetype)*60);
	int front = 0;
	int rear = 0;
	queue[0] = *root;
	int index = 0;
	tree_to_array(queue, &front, &rear, &index);

	for(int i = 0; i < 31; i++){
		printf("%d", file_array[i].valid);
	}

	FILE * fd = fopen("file_structure.bin", "wb");

	FILE * fd1 = fopen("super.bin", "wb");

	fwrite(file_array, sizeof(filetype)*31, 1, fd);
	fwrite(&spblock,sizeof(superblock),1,fd1);

	fclose(fd);
	fclose(fd1);

	printf("\n");
}

void initialize_root_directory() {

	spblock.inode_bitmap[1]=1; //marking it with 0
	root = (filetype *) malloc (sizeof(filetype));

	strcpy(root->path, "/");
	strcpy(root->name, "/");

	root -> children = NULL;
	root -> num_children = 0;
	root -> parent = NULL;
	root -> num_links = 2;
	root -> valid = 1;
	strcpy(root->test, "test");
	//root -> type = malloc(10);
	strcpy(root -> type, "directory");

	root->c_time = time(NULL);
	root->a_time = time(NULL);
	root->m_time = time(NULL);
	root->b_time = time(NULL);

	root -> permissions = S_IFDIR | 0777;

	root -> size = 0;
	root->group_id = getgid();
	root->user_id = getuid();


	root -> number = 2;
	//root -> size = 0;
	root -> blocks = 0;

	save_contents();
}




filetype * filetype_from_path(char * path){
	char curr_folder[100];
	char * path_name = malloc(strlen(path) + 2);

	strcpy(path_name, path);

	filetype * curr_node = root;

	fflush(stdin);

	if(strcmp(path_name, "/") == 0)
		return curr_node;

	if(path_name[0] != '/'){
		printf("INCORRECT PATH\n");
		exit(1);
	}
	else{
		path_name++;
	}

	if(path_name[strlen(path_name)-1] == '/'){
		path_name[strlen(path_name)-1] = '\0';
	}

	char * index;
	int flag = 0;

	while(strlen(path_name) != 0){
		index = strchr(path_name, '/');
		
		if(index != NULL){
			strncpy(curr_folder, path_name, index - path_name);
			curr_folder[index-path_name] = '\0';
			
			flag = 0;
			for(int i = 0; i < curr_node -> num_children; i++){
				if(strcmp((curr_node -> children)[i] -> name, curr_folder) == 0){
					curr_node = (curr_node -> children)[i];
					flag = 1;
					break;
				}
			}
			if(flag == 0)
				return NULL;
		}
		else{
			strcpy(curr_folder, path_name);
			flag = 0;
			for(int i = 0; i < curr_node -> num_children; i++){
				if(strcmp((curr_node -> children)[i] -> name, curr_folder) == 0){
					curr_node = (curr_node -> children)[i];
					return curr_node;
				}
			}
			return NULL;
		}
		path_name = index+1;
	}

}

int find_free_inode(){
	for (int i = 2; i < 100; i++){
		if(spblock.inode_bitmap[i] == '0'){
			spblock.inode_bitmap[i] = '1';
		}
		return i;
	}
}

int find_free_db(){
	for (int i = 1; i < 100; i++){
		if(spblock.inode_bitmap[i] == '0'){
			spblock.inode_bitmap[i] = '1';
		}
		return i;
	}
}

void add_child(filetype * parent, filetype * child){
	(parent -> num_children)++;

	parent -> children = realloc(parent -> children, (parent -> num_children)*sizeof(filetype *));

	(parent -> children)[parent -> num_children - 1] = child;
}

static int mymkdir(const char *path, mode_t mode) {
	printf("MKDIR\n");

	int index = find_free_inode();

	filetype * new_folder = malloc(sizeof(filetype));

	char * pathname = malloc(strlen(path)+2);
	strcpy(pathname, path);

	char * rindex = strrchr(pathname, '/');

	//new_folder -> name = malloc(strlen(pathname)+2);
	strcpy(new_folder -> name, rindex+1);
	//new_folder -> path = malloc(strlen(pathname)+2);
	strcpy(new_folder -> path, pathname);



	*rindex = '\0';

	if(strlen(pathname) == 0)
	strcpy(pathname, "/");

	new_folder -> children = NULL;
	new_folder -> num_children = 0;
	new_folder -> parent = filetype_from_path(pathname);
	new_folder -> num_links = 2;
	new_folder -> valid = 1;
	strcpy(new_folder -> test, "test");

	if(new_folder -> parent == NULL)
		return -ENOENT;

	//printf(";;;;%p;;;;\n", new_folder);

	add_child(new_folder->parent, new_folder);

	//new_folder -> type = malloc(10);
	strcpy(new_folder -> type, "directory");

	new_folder->c_time = time(NULL);
	new_folder->a_time = time(NULL);
	new_folder->m_time = time(NULL);
	new_folder->b_time = time(NULL);

	new_folder -> permissions = S_IFDIR | 0777;

	new_folder -> size = 0;
	new_folder->group_id = getgid();
	new_folder->user_id = getuid();


	new_folder -> number = index;
	new_folder -> blocks = 0;


	save_contents();

	return 0;

}


int myreaddir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ){
	printf("READDIR\n");

	filler(buffer, ".", NULL, 0 );
	filler(buffer, "..", NULL, 0 );

	char * pathname = malloc(strlen(path)+2);
	strcpy(pathname, path);

	filetype * dir_node = filetype_from_path(pathname);

	if(dir_node == NULL){
		return -ENOENT;
	}
	else{
		dir_node->a_time=time(NULL);
		for(int i = 0; i < dir_node->num_children; i++){
			printf(":%s:\n", dir_node->children[i]->name);
			filler( buffer, dir_node->children[i]->name, NULL, 0 );
		}
	}

	return 0;
}


static int mygetattr(const char *path, struct stat *statit) {
	char *pathname;
	pathname=(char *)malloc(strlen(path) + 2);

	strcpy(pathname, path);

	printf("GETATTR %s\n", pathname);

	filetype * file_node = filetype_from_path(pathname);
	if(file_node == NULL)
		return -ENOENT;

	statit->st_uid = file_node -> user_id; // The owner of the file/directory is the user who mounted the filesystem
	statit->st_gid = file_node -> group_id; // The group of the file/directory is the same as the group of the user who mounted the filesystem
	statit->st_atime = file_node -> a_time; // The last "a"ccess of the file/directory is right now
	statit->st_mtime = file_node -> m_time; // The last "m"odification of the file/directory is right now
	statit->st_ctime = file_node -> c_time;
	statit->st_mode = file_node -> permissions;
	statit->st_nlink = file_node -> num_links + file_node -> num_children;
	statit->st_size = file_node -> size;
	statit->st_blocks = file_node -> blocks;

	return 0;
}

int myrmdir(const char * path){

	char * pathname = malloc(strlen(path)+2);
	strcpy(pathname, path);

	char * rindex = strrchr(pathname, '/');

	char * folder_delete = malloc(strlen(rindex+1)+2);

	strcpy(folder_delete, rindex+1);

	*rindex = '\0';

	if(strlen(pathname) == 0)
		strcpy(pathname, "/");

	filetype * parent = filetype_from_path(pathname);

	if(parent == NULL)
		return -ENOENT;

	if(parent -> num_children == 0)
		return -ENOENT;

	filetype * curr_child = (parent -> children)[0];
	int index = 0;
	while(index < (parent -> num_children)){
		if(strcmp(curr_child -> name, folder_delete) == 0){
			break;
		}
		index++;
		curr_child = (parent -> children)[index];
	}

	if(index < (parent -> num_children)){
		if(((parent -> children)[index] -> num_children) != 0)
			return -ENOTEMPTY;
		for(int i = index+1; i < (parent -> num_children); i++){
			(parent -> children)[i-1] = (parent -> children)[i];
		}
		(parent -> num_children) -= 1;
	}

	else{
		return -ENOENT;
	}

	save_contents();

	return 0;

}

int myrm(const char * path){

	char * pathname = malloc(strlen(path)+2);
	strcpy(pathname, path);

	char * rindex = strrchr(pathname, '/');

	char * folder_delete = malloc(strlen(rindex+1)+2);

	strcpy(folder_delete, rindex+1);

	*rindex = '\0';

	if(strlen(pathname) == 0)
		strcpy(pathname, "/");

	filetype * parent = filetype_from_path(pathname);

	if(parent == NULL)
		return -ENOENT;

	if(parent -> num_children == 0)
		return -ENOENT;

	filetype * curr_child = (parent -> children)[0];
	int index = 0;
	while(index < (parent -> num_children)){
		if(strcmp(curr_child -> name, folder_delete) == 0){
			break;
		}
		index++;
		curr_child = (parent -> children)[index];
	}

	if(index < (parent -> num_children)){
		if(((parent -> children)[index] -> num_children) != 0)
			return -ENOTEMPTY;
		for(int i = index+1; i < (parent -> num_children); i++){
			(parent -> children)[i-1] = (parent -> children)[i];
		}
		(parent -> num_children) -= 1;
	}

	else{
		return -ENOENT;
	}

	save_contents();

	return 0;

}


int mycreate(const char * path, mode_t mode, struct fuse_file_info *fi) {

	printf("CREATEFILE\n");

	int index = find_free_inode();

	filetype * new_file = malloc(sizeof(filetype));

	char * pathname = malloc(strlen(path)+2);
	strcpy(pathname, path);

	char * rindex = strrchr(pathname, '/');

	strcpy(new_file -> name, rindex+1);
	strcpy(new_file -> path, pathname);

	*rindex = '\0';

	if(strlen(pathname) == 0)
		strcpy(pathname, "/");

	new_file -> children = NULL;
	new_file -> num_children = 0;
	new_file -> parent = filetype_from_path(pathname);
	new_file -> num_links = 0;
	new_file -> valid = 1;

	if(new_file -> parent == NULL)
	return -ENOENT;

	add_child(new_file->parent, new_file);

	//new_file -> type = malloc(10);
	strcpy(new_file -> type, "file");

	new_file->c_time = time(NULL);
	new_file->a_time = time(NULL);
	new_file->m_time = time(NULL);
	new_file->b_time = time(NULL);

	new_file -> permissions = S_IFREG | 0777;

	new_file -> size = 0;
	new_file->group_id = getgid();
	new_file->user_id = getuid();


	new_file -> number = index;

	for(int i = 0; i < 16; i++){
		(new_file -> datablocks)[i] = find_free_db();
	}

	//new_file -> size = 0;
	new_file -> blocks = 0;

	save_contents();

	return 0;
}


int myopen(const char *path, struct fuse_file_info *fi) {
	printf("OPEN\n");

	char * pathname = malloc(sizeof(path)+1);
	strcpy(pathname, path);

	filetype * file = filetype_from_path(pathname);

	return 0;
}

int myread(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi) {

	printf("READ\n");

	char * pathname = malloc(sizeof(path)+1);
	strcpy(pathname, path);

	filetype * file = filetype_from_path(pathname);
	if(file == NULL)
		return -ENOENT;

	else{
		char * str = malloc(sizeof(char)*1024*(file -> blocks));

		printf(":%ld:\n", file->size);
		strcpy(str, "");
		int i;
		for(i = 0; i < (file -> blocks) - 1; i++){
			strncat(str, &spblock.datablocks[block_size*(file -> datablocks[i])], 1024);
			printf("--> %s", str);
		}
		strncat(str, &spblock.datablocks[block_size*(file -> datablocks[i])], (file -> size)%1024);
		printf("--> %s", str);
		//strncpy(str, &spblock.datablocks[block_size*(file -> datablocks[0])], file->size);
		strcpy(buf, str);
	}
	return file->size;
}

int myaccess(const char * path, int mask){
	return 0;
}


int myrename(const char* from, const char* to) {
	printf("RENAME: %s\n", from);
	printf("RENAME: %s\n", to);

	char * pathname = malloc(strlen(from)+2);
	strcpy(pathname, from);

	char * rindex1 = strrchr(pathname, '/');

	filetype * file = filetype_from_path(pathname);

	*rindex1 = '\0';

	char * pathname2 = malloc(strlen(to)+2);
	strcpy(pathname2, to);

	char * rindex2 = strrchr(pathname2, '/');


	if(file == NULL)
		return -ENOENT;

	//file -> name = realloc(file -> name, strlen(rindex2+1)+2);
	strcpy(file -> name, rindex2+1);
	//file -> path = realloc(file -> path, strlen(to)+2);
	strcpy(file -> path, to);



	printf(":%s:\n", file->name);
	printf(":%s:\n", file->path);

	save_contents();

	return 0;
}

int mytruncate(const char *path, off_t size) {
	return 0;
}

int mywrite(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

	printf("WRITING\n");

	char * pathname = malloc(sizeof(path)+1);
	strcpy(pathname, path);

	filetype * file = filetype_from_path(pathname);
	if(file == NULL)
		return -ENOENT;

	int indexno = (file->blocks)-1;

	if(file -> size == 0){
		strcpy(&spblock.datablocks[block_size*((file -> datablocks)[0])], buf);
		file -> size = strlen(buf);
		(file -> blocks)++;
	}
	else{
		int currblk = (file->blocks)-1;
		int len1 = 1024 - (file -> size % 1024);
		if(len1 >= strlen(buf)){
			strcat(&spblock.datablocks[block_size*((file -> datablocks)[currblk])], buf);
			file -> size += strlen(buf);
			printf("---> %s\n", &spblock.datablocks[block_size*((file -> datablocks)[currblk])]);
		}
		else{
			char * cpystr = malloc(1024*sizeof(char));
			strncpy(cpystr, buf, len1-1);
			strcat(&spblock.datablocks[block_size*((file -> datablocks)[currblk])], cpystr);
			strcpy(cpystr, buf);
			strcpy(&spblock.datablocks[block_size*((file -> datablocks)[currblk+1])], (cpystr+len1-1));
			file -> size += strlen(buf);
			printf("---> %s\n", &spblock.datablocks[block_size*((file -> datablocks)[currblk])]);
			(file -> blocks)++;
		}

	}
	save_contents();

	return strlen(buf);
}

static struct fuse_operations operations =
{
	.mkdir=mymkdir,
	.getattr=mygetattr,
	.readdir=myreaddir,
	.rmdir=myrmdir,
	.open=myopen,
	.read=myread,
	.write=mywrite,
	.create=mycreate,
	.rename=myrename,
	.unlink=myrm,
};

int main( int argc, char *argv[] ) {
	FILE *fd = fopen("file_structure.bin", "rb");
	if(fd){
	printf("LOADING\n");
	fread(&file_array, sizeof(filetype)*31, 1, fd);

	int child_startindex = 1;
	file_array[0].parent = NULL;

	for(int i = 0; i < 6; i++){
		file_array[i].num_children = 0;
		file_array[i].children = NULL;
		for(int j = child_startindex; j < child_startindex + 5; j++){
			if(file_array[j].valid){
				add_child(&file_array[i], &file_array[j]);
			}
		}
		child_startindex += 5;
	}

		root = &file_array[0];

		FILE *fd1 = fopen("super.bin", "rb");
		fread(&spblock,sizeof(superblock),1,fd1);
	}
	else{
		initialize_superblock();
		initialize_root_directory();
	}

	return fuse_main(argc, argv, &operations, NULL);
}
