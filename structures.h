#include "sp.h"



#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>

#include <errno.h>
#include <time.h>

#define PORT         10150


#define MAX_MESS_LEN 1400
#define SPREAD_PORT "10150"
#define MAX_VSSETS      10
#define MAX_MEMBERS     100




#define BUF_SIZE 6500

/*Spread USER*/
//#define SPREAD_USER ""
#define GROUP "rdeshpa33amehta26"

#define SERVER_GRP "server_grp"

//#include "linked_list.h"
/*Different types of user requests user makes*/
typedef enum request{
	MSG, LIKE, UNLIKE, JOIN, LEAVE, HISTORY, VIEW,CONNECT
}request;

typedef enum list_type{
	LIST_UPDATE, LIST_LINE, LIST_META 
}list_type;


/*Declarations of various types of nodes here*/
typedef struct node{

	void* data;
	struct node* next;
}node;

typedef struct linked_list{
	node* head;
	node* tail;
	list_type linked_list_type;
}linked_list;
/*Lamport Timestamp*/
typedef struct LTS{
	int LTS_counter;
	int LTS_server_id;

}LTS;

/*Update structure for like/unlike a line number*/
typedef struct like_packet{

	int like_packet_line_no;
	struct LTS like_packet_line_no_lts;
	char like_packet_user[80];
}like_packet;

/*Update structure for joining/leaving a packet*/
typedef struct join_packet{

	char *join_packet_user;
}join_packet;

/*Line packet which consists of the portion of the packet that needs to be sent
 * to the clients*/
typedef struct line_packet{

	char line_packet_user[80];
	char line_packet_message[80];
	int line_packet_likes;
	int line_packet_line_no;
	struct LTS line_packet_lts;
}line_packet;




typedef union union_update_data{
	struct line_packet data_line;
	struct like_packet data_like;
	struct join_packet data_join;

}union_update_data;


/*Update structure, for all kinds of updates*/
typedef struct update{

	request update_type;
	struct LTS update_lts;
	char update_chat_room[80];
	union union_update_data update_data;


}update;


/*Structure of message/line. We maintain a linkedlist of these 
 * per chat group*/
typedef struct line{

	struct line_packet line_content;
	linked_list* line_meta;
}line;

/*Data to be inserted into line descriptor node*/
typedef struct meta{

	//user ID of user who has liked a particular message
	char meta_user[80];

}meta;



/*My variables data structure client*/
typedef struct my_variables_client{

	int machine_id;
	int my_ip;
	sp_time timeout;
	char private_group[80];
}client_variables;

/*My variables data structure server*/
typedef struct my_variables_server{

	int machine_id;
	int my_ip;
	sp_time timeout;
	char private_group[80];
}server_variables;



char *public_server_grps[6]={"dummy","s1","s2","s3","s5","s6"};
char *private_server_grps[6]={"dummy","server1","server2","server3","server5","server6"};



node* get_node(list_type);
linked_list* get_linked_list(list_type);
node* create_meta(char* meta);
void test_meta_list();
void append(linked_list*, node*);
node* create_line(char* user, char* message, int likes, int line_no, LTS lts);

LTS create_LTS(int counter, int server_id);

like_packet create_like_packet(int line_no, LTS lts, char* user);
void print_meta(linked_list *meta_ll);

void test_update_list();
node* create_update( request update_type, LTS lts, char* chat_room, union_update_data data);

void print_update(linked_list *update_ll);

void print_line(linked_list *line_ll);

void test_line_list();
void delete(linked_list *list,node* location);
void insert(linked_list *list, node* new_node, node* location);
node* seek_user(linked_list *list, char* user,int *ret_val);
node* seek(linked_list *list, LTS lts , int *ret_val);