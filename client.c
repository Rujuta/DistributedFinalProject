#include "sp.h"
#include "structures.h"

#define debug 1
#define clear() printf("\033[H\033[J")


void process_message(char* mess,client_variables *local_var);

/*This will be joined by the client initially so that it can check if server is alive and get membership msgs*/
char *public_server_grps[6]={"dummy","s1","s2","s3","s5","s6"};

/*Sent by client to server to send all other msgs*/
char *private_server_grps[6]={"dummy","server1","server2","server3","server5","server6"};

/*Packet of specified type created for sending*/
request_packet* create_packet(request type,char* data, LTS lts, client_variables* local_var);

/*Multicasts a packet*/
void send_packet(request_packet *packet, client_variables* local_var);

/*The function that is called to accept and process user input*/
static  void    User_command(int, int, void*);

/*User connecting to spread*/
static  char    User[SIZE]; //machine ID of client connecting to spread

/*Spread daemon - port */
static  char    Spread_name[SIZE]; 

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

void connect_to_spread(client_variables *local_var);

static void Usage();

//int init_prompt();


void process_input(char* input, client_variables *local_var);


FILE *log1=NULL;

int main(int argc, char* argv[]){

	/*Holds the client specific variables*/
	client_variables local_var;
	if(argc<2){
		Usage();
	}

	/*Open file for logging*/
	log1=fopen(argv[1],"w");

	if(log1 == NULL){
		printf("\nError opening file");
		exit(1);
	}

	/*Assign my local client ID to me*/
	Assign(argv);
	
	/*Set local variables in local_var*/
	local_var.machine_id=atoi(argv[1]);
	local_var.timeout.sec=5;
	local_var.timeout.usec=0;
	/*Initialize an empty linkedlist for msgs*/
	local_var.msg_list=get_linked_list(LIST_LINE);

	/*Initialize my chatroom to be the empty string*/
	sprintf(local_var.my_chatroom,"%s","\0");

	/*Connect to spread*/
	connect_to_spread(&local_var);

	/*Initialize Spread event handling system*/
	E_init();

	/*Attach Read message callback function with the spread mailbox to monitor incoming messages*/
	E_attach_fd( Mbox, READ_FD, Read_message, 0, &local_var, HIGH_PRIORITY );
	E_attach_fd( 0, READ_FD, User_command, 0, &local_var, LOW_PRIORITY );

	/*Main control loop of spread events*/
	E_handle_events();

	return( 0 );

}



void connect_to_spread(client_variables *local_var){

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

}

void print_display(){


	printf("\nYou can do the following things:\n");
	printf("\n1.Append\n2.Like\n3.Dislike\n3.Change user/login as other\n4.View servers\n5.View history\n");	

}
static  void    Bye()
{
	To_exit = 1;

	printf("\nBye.\n");

	SP_disconnect( Mbox );

	exit( 0 );
}



/*This function creates a packet of the type specified with the data specified and then calls the send function on it*/
request_packet* create_packet(request type,char* data, LTS lts, client_variables* local_var){

	request_packet *new_packet=(request_packet*)malloc(sizeof(request_packet));
	new_packet->request_packet_type=type;
	new_packet->request_packet_lts.LTS_counter=lts.LTS_counter;
	new_packet->request_packet_lts.LTS_server_id=lts.LTS_server_id;
	sprintf(new_packet->request_packet_data,"%s",data);
	sprintf(new_packet->request_packet_user,"%s",local_var->username);
	sprintf(new_packet->request_packet_chatroom,"%s",local_var->my_chatroom);	
	/*Once packet is created, just send the packet to the server*/
	send_packet(new_packet,local_var);

}

void process_input(char* input, client_variables *local_var){

	int ret;
	LTS my_lts;
	my_lts.LTS_counter=-1;
	my_lts.LTS_server_id=-1;
	char group[SIZE];
	if (input[strlen(input) - 1] == '\n') {
		input[strlen(input) - 1] = '\0';
	}
	switch(input[0]){
	
		case 'u': sprintf(local_var->username,"%s",&input[2]); 
			  printf("\nWelcome %s\n",local_var->username);
			break;
		case 'a': create_packet(MSG,&input[2],my_lts,local_var);
			break;
		case 'r': //find LTS of line in memory
			  //create LTS structure here
			  create_packet(UNLIKE,NULL,my_lts,local_var);
			break;
		case 'l'://find LTS of line in memory
			 //create LTS structre here
			 create_packet(LIKE, NULL, my_lts, local_var);
			break;
		case 'h': create_packet(HISTORY,NULL,my_lts,local_var);
			break;
		case 'v':create_packet(VIEW,NULL,my_lts,local_var);
			break;
		case 'c': //read which server to connect to
			  
			  //join the server group with that server
			  //check if that server exists in that group
			  //if exists - do we just chuck sending the connect request? What does the server do on getting it? 
			  //Does it create an entry for the clients connected to it? 
			
			 /*Join the public group on spread, all clients will be connected to server in this group*/
			local_var->my_server=atoi(&input[2]);
			ret = SP_join( Mbox, public_server_grps[local_var->my_server]);
			if( ret < 0 ) SP_error( ret );


			 break;
		case 'j': sscanf(&input[2],"%s",local_var->my_chatroom);
			  //sprintf(local_var->my_chatroom,"%s",&input[2]);
			  create_packet(JOIN,&input[2],my_lts,local_var);

				
			break;

		default: printf("\nPlease select a valid input\n");
			 break;
	
	}

}



/*There will be a screen data structure, on
 * receiving updated data from server, this structure will 
 * be updated and user will see new set of messages*/
void refresh_screen(client_variables *local_var){

	if(debug){
		printf("\nRefreshing screen!\n");
	
	}
	node* temp=local_var->msg_list->head;
	int line_no=1;
	clear();
	printf("\n Room : %s",local_var->my_chatroom);
	printf("\nAttendees: ");
	node* u=local_var->users_in_room->head;
	while(u!=NULL){
	
		meta* v= (meta*) u->data;
		printf("%s ",v->meta_user);
		u=u->next;
	
	}
	//also need to print the users in the room.
	
	while(temp!=NULL){

		line* my_data2=(line*)temp->data;
		

		printf("\n %d %s ",line_no,my_data2->line_content.line_packet_user);
		printf("%s ",  my_data2->line_content.line_packet_message);
		if(my_data2->line_content.line_packet_likes>0)
			printf("\tLikes : %d",  my_data2->line_content.line_packet_likes);
//		printf("\n");
		temp=temp->next;
		line_no++;
		fflush(stdout);
	}	


}





/*Creates a Username for this client, with which it will try and connect to spread - something like a client id*/
static  void    Assign(char *argv[])
{
	if(debug){
		printf("in assign");
	}
	strcat(User,"cl");
	strcat( User, argv[1] );
	if(debug){
		fprintf(log1,"In assign %s",argv[1]);
	}
	sprintf( Spread_name, SPREAD_PORT);


}

static void Usage(){

	printf("\nUsage: <client_id>\n");
	exit(1);
}


static  void    User_command(int a, int b, void* local_var_arg)
{
	char    command[SIZE];
	char    mess[MAX_MESS_LEN];
	int     ret;
	int     i;
	client_variables *local_var=(client_variables*)local_var_arg;
	for( i=0; i < sizeof(command); i++ ) command[i] = 0;
	if( fgets( command, SIZE, stdin ) == NULL )
		Bye();

	process_input(command,local_var);	
	printf("\nUser> ");
	fflush(stdout);

}


static  void    Read_message(int a, int b, void *local_var_arg)
{

	static  char     mess[MAX_MESS_LEN]; //arrived msg stored here
	char             sender[MAX_GROUP_NAME]; //gets the sender details
	char             target_groups[MAX_MEMBERS][MAX_GROUP_NAME]; //on regular membership msg received, this gets filled with member names
	membership_info  memb_info; //structure to store membership info
	vs_set_info      vssets[MAX_VSSETS];
	unsigned int     my_vsset_index;
	int              num_vs_sets;
	char             members[MAX_MEMBERS][MAX_GROUP_NAME]; 
	int              num_groups; //number of members in a group when membinfo is received
	int              service_type; // agreed/fifo/causal etc, carried different meanings
	int16            mess_type; // membership/regular etc
	int              endian_mismatch;
	int              i,j;
	int              ret;

	client_variables *local_var;
	local_var=(client_variables*) local_var_arg;

	service_type = 0;

	ret = SP_receive( Mbox, &service_type, sender, 100, &num_groups, target_groups,
			&mess_type, &endian_mismatch, sizeof(mess), mess );
	if( ret < 0 )
	{


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
			fprintf(log1,"message from %s, of type %d,(%d bytes)\n",
					sender, mess_type,  ret );
		}
		//here message of type node is received. 
		//It needs to be added to message list
		process_message(mess,local_var);


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
			if(debug){
				fprintf(log1,"Received REGULAR membership for group %s with %d members, where I am member %d:\n",
					sender, num_groups, mess_type );
			}
			if( Is_caused_join_mess( service_type ) )
			{
				//printf("Due to join of  %s\n",  memb_info.changed_member);
				fprintf(log1,"Due to the JOIN of %s\n", memb_info.changed_member );

				/*Here if it is a message of type connect - then check if server exists in the group just joined*/
				int s_cnt=0;
				int flag=0;
				char *str;
				for(s_cnt=0;s_cnt<num_groups; s_cnt++){
				
					str=strtok(target_groups[s_cnt],"#");
					if(strcmp(str,public_server_grps[local_var->my_server])==0){
					
						//printf("\nServer exists");
						flag=1;
						
					}
				}
				if(flag==1){
					printf("\nServer exists, you can now login, and join any chat room!\n");
				}
				else{
					printf("\nConnect to another server, looks like the one you entered is dead\n");
				}

				fflush(stdout);
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

/*Multicasts a packet to the server, in our case to the server's public group to which ONLY that server is connected*/
void send_packet(request_packet *packet,client_variables *local_var){

	int ret;
	ret= SP_multicast( Mbox, AGREED_MESS,private_server_grps[local_var->my_server], 1, sizeof(request_packet), (char*)packet );
	if(debug)
		fprintf(log1,"\nSending a connect packet\n");


	if( ret < 0 )
	{
		SP_error( ret );
		Bye();
	}


}


void process_message(char* mess,client_variables *local_var){


	response_packet *new_response;
	new_response = (response_packet*) mess;
	int ret;
	switch(new_response->response_packet_type){
	
		case R_ACK: // here is an ack to join a group, do an sp_join on receive
			printf("\nGot ACK");
			ret = SP_join( Mbox, local_var->my_chatroom);
			if( ret < 0 ) SP_error( ret );
			if(debug){
				fprintf(log1,"\nJoined  group");
				fflush(log1);
			}
			/*Put users in chat group in list of users in chat group in local var*/
			local_var->users_in_room=get_linked_list(LIST_META);
			int i =0;
			
			while(strcmp(new_response->data.users[i],"\0")!=0){
			
				node *new_user=create_meta(new_response->data.users[i]);
				append(local_var->users_in_room,new_user);
				i++;

			}
			
			break;
		case R_HISTORY:
			break;
		case R_MSG:
			//printf("\ngot meessage : %s",new_response->data.line.line_packet_message);
			;
			node * n1 = create_line(new_response->data.line.line_packet_user,new_response->data.line.line_packet_message,new_response->data.line.line_packet_likes,new_response->data.line.line_packet_lts);
			append(local_var->msg_list,n1);
			//print_line(local_var->msg_list);
			refresh_screen(local_var);

			break;
		case R_VIEW:
			break;
	
	}


}
