#include "sp.h"
#include "structures.h"
#define debug 1


/*The function that is called to accept and process user input*/
static  void    User_command();

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

int init_prompt();


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
	//E_attach_fd( 0, READ_FD, User_command, 0, NULL, LOW_PRIORITY );

	if(init_prompt()==0){

		//Display user prompt 
		E_attach_fd( 0, READ_FD, User_command, 0, NULL, LOW_PRIORITY );

	}
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


int init_prompt(){
	char input[MAX_MESS_LEN];
	char server[MAX_MESS_LEN];
	int ret;
	static  char     mess[MAX_MESS_LEN];
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

	service_type = 0;
	printf("Connect to a server:\nUsage:\nc <server_id>");
	fgets(input,MAX_MESS_LEN,stdin);
	switch(input[0]){

		case 'c':
			sscanf(&input[2],"%s",server);
			ret= SP_multicast( Mbox, AGREED_MESS,private_server_grps[atoi(server)], 1, sizeof(input), (char*)input );
			if(debug)
				fprintf(log1,"\nSending a connect packet\n");


			if( ret < 0 )
			{
				SP_error( ret );
				Bye();
			}
			//E_delay(timeout);
			ret = SP_receive( Mbox, &service_type, sender, 100, &num_groups, target_groups,
					&mess_type, &endian_mismatch, sizeof(mess), mess );

			if(Is_regular_mess( service_type )){

				mess[ret] = 0;
				printf("message from %s, of type %d, (endian %d) to %d groups \n(%d bytes): %s\n",
						sender, mess_type, endian_mismatch, num_groups, ret, mess );
				fprintf(log1,"%s","\nCan start sending messages now");

				/*Join the public group on spread, all clients will be connected to server in this group*/
				ret = SP_join( Mbox, public_server_grps[atoi(server)] );
				if( ret < 0 ) SP_error( ret );

				return 0;

			}
			else{

				printf("\nThis server is probably dead, please connect to another server");
				init_prompt();
			}
			break;



	}
	return 0;

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


static  void    User_command()
{
	char    command[130];
	char    mess[MAX_MESS_LEN];
	char    group[80];
	char    groups[10][MAX_GROUP_NAME];
	int     num_groups;
	unsigned int    mess_len;
	int     ret;
	int     i;

	for( i=0; i < sizeof(command); i++ ) command[i] = 0;
	if( fgets( command, 130, stdin ) == NULL )
		Bye();

	switch( command[0] )
	{
		default:
			printf("\nUnknown commnad\n");
			//Print_menu();
			print_display();	

			break;
	}
	printf("\nUser> ");
	fflush(stdout);

}


static  void    Read_message(int a, int b, void *local_var_arg)
{

	static  char     mess[MAX_MESS_LEN];
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

void login(client_variables *local_var){

	char username[MAX_MESS_LEN];
	printf("\nPlease login with username, Usage: u <username>");
	fgets(username,"%s",stdin);
	ret= SP_multicast( Mbox, AGREED_MESS,local_var->my_server, 1, sizeof(username), (char*)input );
	if(debug)
		fprintf(log1,"\nSending a connect packet\n");


	if( ret < 0 )
	{
		SP_error( ret );
		Bye();
	}


}

