#include "sp.h"
#include "structures.h"
#define SPREAD_PORT "10150"
#define debug 1

#define MAX_MESSLEN     102400
#define MAX_VSSETS      10
#define MAX_MEMBERS     100
#define SPREAD_PORT "10150"
#define MAXINT 2147483647


char *public_server_grps[6]={"dummy","s1","s2","s3","s5","s6"};
char *private_server_grps[6]={"dummy","server1","server2","server3","server5","server6"};


response_packet* create_response_packet(response type, server_variables* local_var);

void process_message(char* mess,char* sender, server_variables *local_var);
/*User connecting to spread*/
static  char    User[80]; //machine ID of client connecting to spread

/*Spread daemon - port */
static  char    Spread_name[80]; 

/*Spread assigns this on setting up a connection with a user*/
static  char    Private_group[MAX_GROUP_NAME];

/*File descriptor that needs to be monitored for all spread messages*/
static  mailbox Mbox;

static int To_exit=0;

/*Function that reads messages and processes them*/
static  void    Read_message(int,int, void*);


static  void    Assign(char *argv[] );

/*Exits*/
static  void    Bye();

void connect_to_spread(server_variables *local_var);


static void Usage();

/*Function that reads messages and processes them*/
static  void    Read_message(int,int, void*);



FILE *log1=NULL;

int main(int argc, char* argv[]){

	server_variables local_var;
	int ret;
	if(argc<2){
		Usage();
	}

	char filename[80];
	sprintf(filename,'\0');
	strcat(filename,"s");
	strcat(filename,argv[1]);

	/*Open file for logging*/
	log1=fopen(filename,"w");

	if(log1 == NULL){
		printf("\nError opening file");
		exit(1);
	}
	Assign(argv);
	/*Set local variables in local_var*/
	local_var.machine_id=atoi(argv[1]);
	local_var.timeout.sec=5;
	local_var.timeout.usec=0;

	//Set my local counter 
	local_var.my_lts.LTS_counter=0;
	local_var.my_lts.LTS_server_id=local_var.machine_id;

	/*This will actually be a pointer to a list, each connecting to another list - each list standing
	 * for one chat room*/
	local_var.chat_lists=(linked_list*)get_linked_list(LIST_CHATROOM);

	/*Here you will load old DATA of chatrooms into the chatlists structure*/
	//TODO
	connect_to_spread(&local_var);
	/*Initialize Spread event handling system*/
	E_init();

	/*Attach Read message callback function with the spread mailbox to monitor incoming messages*/
	E_attach_fd( Mbox, READ_FD, Read_message, 0, &local_var, HIGH_PRIORITY );


	/*Main control loop of spread events*/
	E_handle_events();

	return( 0 );

	//display_prompt();
}






void connect_to_spread(server_variables *local_var){

	int ret;
	/*Connect to spread*/
	ret = SP_connect_timeout( Spread_name, User, 0, 1, &Mbox, Private_group, local_var->timeout);
	if( ret != ACCEPT_SESSION )
	{
		SP_error( ret );
		Bye();
	}
	printf("User: connected to %s with private group %s\n", Spread_name, Private_group,local_var->timeout );
	sprintf(local_var->private_group,Private_group);

	if(debug){
		fprintf(log1,"fd:%d",Mbox);
	}


	/*Join the group server_grp on spread*/
	ret = SP_join( Mbox, SERVER_GRP );
	if( ret < 0 ) SP_error( ret );

	/*Join the public group on spread, all clients will be connected to server in this group*/
	ret = SP_join( Mbox, public_server_grps[local_var->machine_id] );
	if( ret < 0 ) SP_error( ret );



	/*Join the private group on spread*/
	ret = SP_join( Mbox, private_server_grps[local_var->machine_id] );
	if( ret < 0 ) SP_error( ret );




}


static  void    Bye()
{
	To_exit = 1;

	printf("\nBye.\n");

	SP_disconnect( Mbox );

	exit( 0 );
}


/*Send a packet to the server client is connected to*/
void send_packet(char *group_name, response_packet *packet, server_variables *local_var){


	int ret;
	ret= SP_multicast( Mbox, AGREED_MESS,group_name, 1, sizeof(response_packet), (char*)packet );
	if(debug)
		printf("%d",packet->response_packet_type);
		fprintf(log1,"\nSending a response packet\n");


	if( ret < 0 )
	{
		SP_error( ret );
		Bye();
	}

	printf("\nLeaving send");
}

/*Create packet of a specified type
 * to send to server*/
void create_packet(){

}

static  void    Assign(char *argv[])
{
	if(debug){
		printf("in assign");
	}
	sprintf(User,'\0');
	strcat(User,"s");
	strcat( User, argv[1] );
	if(debug){
		fprintf(log1,"In assign %s",argv[1]);
	}
	sprintf( Spread_name, SPREAD_PORT);


}

static void Usage(){

	printf("\nUsage: <server_id>\n");
	exit(1);
}


static  void    Read_message(int a, int b, void *local_var_arg)
{

	static  char     mess[MAX_MESSLEN];
	char             sender[MAX_GROUP_NAME];
	char             target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
	membership_info  memb_info;
	vs_set_info      vssets[MAX_VSSETS];
	unsigned int     my_vsset_index;
	int              num_vs_sets;
	char             members[MAX_MEMBERS][MAX_GROUP_NAME];
	int              num_groups;
	int              service_type;
	int16            mess_type;
	int              endian_mismatch;
	int              i,j;
	int              ret;

	server_variables *local_var;
	local_var=(server_variables*) local_var_arg;

	service_type = 0;

	ret = SP_receive( Mbox, &service_type, sender, 100, &num_groups, target_groups,
			&mess_type, &endian_mismatch, sizeof(mess), mess );
	if( ret < 0 )
	{

		printf("Return Value :%d");

		if(debug){
			fprintf(log1,"\nIn error 1");
		}
		if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
			service_type = DROP_RECV;
			printf("\n========Buffers or Groups too Short=======\n");
			ret = SP_receive( Mbox, &service_type, sender, MAX_MEMBERS, &num_groups, target_groups,
					&mess_type, &endian_mismatch, sizeof(mess), mess );
		}
	}
	if (ret < 0 )
	{

		if(debug){
			fprintf(log1,"\nIn error 2");
		}
		if( ! To_exit )
		{
			SP_error( ret );
			printf("\n============================\n");
			printf("\nBye.\n");
		}
		exit( 0 );
	}

	if( Is_regular_mess( service_type ) )
	{
		mess[ret] = 0;
		if(debug){
			printf("message from %s, of type %d,(%d bytes)\n",sender,mess_type,ret);
			fprintf(log1,"message from %s, of type %d,(%d bytes)\n",
					sender, mess_type,  ret );
		}
		process_message(mess,sender,local_var);


	}

	else if( Is_membership_mess( service_type ) )
	{
		ret = SP_get_memb_info( mess, service_type, &memb_info );
		if (ret < 0) {
			printf("BUG: membership message does not have valid body\n");
			SP_error( ret );
			exit( 1 );
		}
		if     ( Is_reg_memb_mess( service_type ) )
		{
			fprintf(log1,"Received REGULAR membership for group %s with %d members, where I am member %d:\n",
					sender, num_groups, mess_type );
			if( Is_caused_join_mess( service_type ) )
			{
				printf("Due to join of  %s\n",  memb_info.changed_member);
				fprintf(log1,"Due to the JOIN of %s\n", memb_info.changed_member );
			}else if( Is_caused_leave_mess( service_type ) ){
				printf("Due to the LEAVE of %s\n", memb_info.changed_member );
			}else if( Is_caused_disconnect_mess( service_type ) ){
				printf("Due to the DISCONNECT of %s\n", memb_info.changed_member );
			}
			else{

				printf("Spread Network Error");
				Bye();
			}


		}
		else if( Is_transition_mess(   service_type ) ) {
			printf("received TRANSITIONAL membership for group %s\n", sender );
		}
		else if( Is_caused_leave_mess( service_type ) ){
			printf("received membership message that left group %s\n", sender );
		}
		else printf("received incorrecty membership message of type 0x%x\n", service_type );
	}


	else if ( Is_reject_mess( service_type ) )
	{
		printf("REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
				sender, service_type, mess_type, endian_mismatch, num_groups, ret, mess );
	}
	else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);



}

void process_message(char* mess,char* sender, server_variables *local_var){

	int ret;
	request_packet *recv_packet;//=malloc(sizeof(request_packet));
	recv_packet=(request_packet*)mess;
	node* temp, *prev, *user_check;
	int ret_val=0;
	/*Increment lts*/
	//local_var->my_lts.LTS_counter++;
	chatroom *croom, *my_data3;
	switch(recv_packet->request_packet_type){

		case MSG: printf("\nGot msg of type append\n");
			  //Here need to find out the user is part of which chat room
			  //Append to that chat room
			  //From message, the chatroom data is obtained
			  if(debug){
			  	printf("\nGoing to seek chatroom %s\n",recv_packet->request_packet_chatroom);
			  }
			  print_chatlist(local_var->chat_lists);
			  fflush(stdout);
			  prev=seek_chatroom(local_var->chat_lists,recv_packet->request_packet_chatroom,&ret_val);
			  printf("\n ret val --%d ",ret_val);
			  fflush(stdout);

			  if(ret_val==1){

	                          local_var->my_lts.LTS_counter++;
				  
				  node* new_line=create_line(recv_packet->request_packet_user,recv_packet->request_packet_data,0, local_var->my_lts);
				  if(prev==NULL){
				  	croom=(chatroom*)local_var->chat_lists->head->data;
				  }
				  else{
				  	croom	= (chatroom*)prev->next->data;
				  } 
				 linked_list* chat_list=croom->chatroom_msgs;
				  append(chat_list,new_line);
				  print_line(chat_list);
				  //print_chatlist(local_var->chat_lists);
				  /*Now send the same msg back to client so that it can refresh screen*/

				  response_packet *line_p= create_response_packet(R_MSG,local_var);
				  line* my_data2=(line*) new_line->data;
				  memcpy(&(line_p->data),&(my_data2->line_content),sizeof(line_packet));

				  send_packet(recv_packet->request_packet_chatroom,line_p,local_var);


			  }
			  else{
				  printf("Chatroom doesn't exist, some error in code\n");
			  }


			  break;
		case UNLIKE:
			  break;
		case JOIN: printf("\nGot a message of join");
			   printf("\nRequest to join chat room: %s",recv_packet->request_packet_data);
			   char group[SIZE];
			   sscanf(recv_packet->request_packet_data,"%s",group);
			   local_var->my_lts.LTS_counter++;
			   

			   /*First check if this chatroom exists in the list
			    * i.e seek the list for the location of the chatroom
			    * if null is returned then the chatroom doesn't exist
			    * in that case do as below */

			   prev= seek_chatroom(local_var->chat_lists,group,&ret_val);

			   response_packet *response1=create_response_packet(R_ACK,local_var);
			   if(ret_val==1){
				   //chat room exists

				   //send all the data in chat room so far
				   if(prev==NULL){
				   	temp=local_var->chat_lists->head;
				   }
				  else{
				   temp=prev->next;
				  }
 				


				   my_data3=(chatroom*)temp->data;

				   user_check=seek_user(my_data3->users,recv_packet->request_packet_user,&ret_val);
				
				   if(ret_val==1){
				   	if(user_check==NULL){
				   		//user exists and is the first user 
						user_check=my_data3->users->head;

									   	}
					else{
					
						user_check=user_check->next;

					}
						//increment count of user
					
						
						((meta*)user_check->data)->cnt++;
				   

				   }
				   else{
				   
					node* user_node=create_meta(recv_packet->request_packet_user);
				   	append(my_data3->users,user_node);
				   }



				   /*Create a list of users and send*/
				   node* current_user=my_data3->users->head;
				   int i=0;
				   while(current_user!=NULL){
				   
					   meta* user_m=(meta*)current_user->data;
					   //char* user_name=user_m->meta_user;
					   sprintf(response1->data.users[i],"%s",user_m->meta_user);
					   current_user=current_user->next;
					   i++;
				   	
				   }
				   sprintf(response1->data.users[i],"%s","\0");
				   
				   
				   /*Send this list to sender*/
				   	   
			   	   send_packet(sender,response1,local_var);
				   

				   //send_packet(group,response1, local_var);
				   //
				   //send previous msgs of chat grp to user
				    linked_list* msgs=my_data3->chatroom_msgs;
				   temp= msgs->head;


				   while(temp!=NULL){
					   
					   response_packet *line_p= create_response_packet(R_MSG,local_var);
					   line* my_data4=(line*)temp->data;
					   //line_packet to_send;
					   memcpy(&(line_p->data),&(my_data4->line_content),sizeof(line_packet));

					   send_packet(sender,line_p,local_var);
					   temp= temp->next;
				   }
			   }
			   else{
				   node* new_chatroom=(node*)create_chatroom(group);
				   append(local_var->chat_lists,new_chatroom);


				    node* user_node=create_meta(recv_packet->request_packet_user);
				    chatroom* newly_created_room=(chatroom*)new_chatroom->data;
				    append(newly_created_room->users,user_node);

				   meta* u=(meta*)newly_created_room->users->head->data;
				   sprintf(response1->data.users[0],"%s",u->meta_user);
				    
				   printf("\n%s",u->meta_user);
				   printf("\n%s",response1->data.users[0]);
				   
				   sprintf(response1->data.users[1],"%s","\0");
				   send_packet(sender,response1,local_var);
				   print_meta(newly_created_room->users);
				   //print_chatlist(local_var->chat_lists);
				   /*Join the chat group first on spread*/
				   ret = SP_join( Mbox, group);
				   if( ret < 0 ) {
					   SP_error( ret );
					   Bye();
				   }

			   }

			   break;
		case HISTORY:
			   break;
		case VIEW:
			   break;
		case LIKE:
			   break;


	}
}

/*This function creates a packet of the type specified with the data specified and then calls the send function on it*/
response_packet* create_response_packet(response type, server_variables* local_var){

	response_packet *new_packet=(response_packet*)malloc(sizeof(response_packet));
	new_packet->response_packet_type=type;
	return new_packet;
}


