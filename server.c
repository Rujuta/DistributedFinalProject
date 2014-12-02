#include "sp.h"
#include "structures.h"
//#include<net_include.h>
#define SPREAD_PORT "10150"
#define debug 1

#define MAX_MESSLEN     102400
#define MAX_VSSETS      10
#define MAX_MEMBERS     100
#define SPREAD_PORT "10150"
#define MAXINT 2147483647



void process_message(char* mess,char *);
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


void display_prompt(){

}






/*There will be a screen data structure, on
 * receiving updated data from server, this structure will 
 * be updated and user will see new set of messages*/
void refresh_screen(){

}

/*When client gets disconnected from server, it displays a message
 * telling the user to connect to some other server*/
void process_disconnected(){

}

/*Connect to specified server*/
void connect_to_server(){

}

/*Get user's keyboard input*/
void get_user_input(){

}

/*Send a packet to the server client is connected to*/
void send_packet(){

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
		process_message(mess,sender);


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

void process_message(char* mess,char* sender){

	int ret;
	switch(mess[0]){

		case 'c': ;
			  char connect[MAX_MESS_LEN];
			  sprintf(connect,"CONNECT");
			  printf("\nGot a message of c\n");
			  ret= SP_multicast( Mbox, AGREED_MESS,sender, 1, sizeof(connect), (char*)connect );
			  if(debug)
				  fprintf(log1,"\nSending a connect packet\n");


			  if( ret < 0 )
			  {
				  SP_error( ret );
				  Bye();
			  }


			  break;

	}

}

