#include "sp.h"
#include "structures.h"
#define SPREAD_PORT "10150"
#define debug 1

#define MAX_MESSLEN     102400
#define MAX_VSSETS      10
#define MAX_MEMBERS     100
#define SPREAD_PORT "10150"
#define MAXINT 2147483647




int check_causality(server_variables* local_var,update* update_packet);

void recover(server_variables *local_var);

void write_to_file(server_variables *local_var, update* update_packet);

void send_users(server_variables *local_var);
void send_my_updates(server_variables *local_var, int min);
void create_merge_packet(server_variables *local_var);
void process_leave_chatroom(char* group, server_variables *local_var, char* user, int new_server);
void reconcile_partition(server_variables *local_var, int server_id);
void handle_user_mappings(server_variables *local_var, int new_server, char* group, char* user, int flag);
void propagate_like_update( server_variables *local_var,request_packet* recv_packet, LTS update_lts, request);
char *public_server_grps[6]={"dummy","s1","s2","s3","s4","s5"};
char *private_server_grps[6]={"dummy","server1","server2","server3","server4","server5"};

void propagate_append_update(server_variables *local_var,line_packet lp, LTS update_lts,char* room);
line_packet process_append(server_variables *local_var,char *croom1, char* user, char* msg_data, LTS line_lts);

void send_packet(char *group_name, response_packet *packet, server_variables *local_var, int flag);
void process_update(char* mess, server_variables *local_var, int flag);
chatroom* process_join(char* group, server_variables *local_var,char *user, int server);
void send_update(server_variables *local_var, update* to_send);
void propagate_join_update(server_variables *local_var,request_packet* recv_packet, LTS update_lts,request);
response_packet* process_client_update(request type, server_variables *local_var, char*, char* ,LTS);
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


void reconcile_merge(server_variables *local_var);



FILE *log1=NULL;

FILE *backup=NULL;

FILE *read_backup=NULL;
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

	/*Open file for backup*/
	char filename2[80];
	sprintf(filename2,'\0');
	strcat(filename2,"backup_");
	strcat(filename2,argv[1]);
	backup=fopen(filename2,"a+b");
	if(backup==NULL){

		printf("\nError opening file");
		exit(1);
	}
	read_backup=fopen(filename2,"r+b");
	if(read_backup==NULL){
		printf("\nError opening file");
		exit(1);

	}

	/*Set local variables in local_var*/
	local_var.machine_id=atoi(argv[1]);
	local_var.timeout.sec=5;
	local_var.timeout.usec=0;

	/*Initialize all members to be 0 first*/
	local_var.current_members[0]=0;
	int l;
	for(l=1;l<6;l++){
		local_var.current_members[l]=0;
	}
	//Set my local counter 
	local_var.my_lts.LTS_counter=0;
	local_var.my_lts.LTS_server_id=local_var.machine_id;

	/*This will actually be a pointer to a list, each connecting to another list - each list standing
	 * for one chat room*/
	local_var.chat_lists=(linked_list*)get_linked_list(LIST_CHATROOM);

	/*Initialize an update list. One for updates to put to file and another one to handle non delivered msgs*/
	local_var.update_list=(linked_list*)get_linked_list(LIST_UPDATE);
	local_var.undelivered_update_list=(linked_list*)get_linked_list(LIST_UPDATE);


	int k=1;
	LTS init;
	init.LTS_counter=-1;
	init.LTS_server_id=-1;
	local_var.my_vector[0]=-1;
	while(k<6){

		local_var.my_vector[k]=-1;
		local_var.server_chats[k]=(linked_list*)get_linked_list(LIST_CHATROOM);

		k++;
	}


	int p=1,r=1;
	for(r=1;r<6;r++){

		for(p=1;p<6;p++){
			local_var.recvd_vectors[r][p]=-1;
		}
	}


	/*Here you will load old DATA of chatrooms into the chatlists structure*/
	//TODO - Make sure you need data structures from files

	//	init_data_structures(&local_var);
	connect_to_spread(&local_var);


	recover(&local_var);	
	/*Initialize Spread event handling system*/
	E_init();

	/*Attach Read message callback function with the spread mailbox to monitor incoming messages*/
	E_attach_fd( Mbox, READ_FD, Read_message, 0, &local_var, HIGH_PRIORITY );


	/*Main control loop of spread events*/
	E_handle_events();

	return( 0 );

	//display_prompt();
}


void recover(server_variables *local_var){

	update *u;
	while(!feof(read_backup)){

		node* new_update=get_node(LIST_UPDATE);
		fread((update*)new_update->data,sizeof(update),1,read_backup);
		if(((update *)new_update->data)->update_lts.LTS_counter==0)
			continue;

		u= (update *)new_update->data;
		/*when this last param is 0 - the server DOESN't send the update*/
		process_update((char*)u,local_var,0);	
	}

	//local_var->update_list->tail=NULL;
	//	print_update(local_var->update_list);

	//	printf("\nDone printing\n");
	//	fflush(stdout);
	//	exit(0);



}


void write_to_file(server_variables* local_var, update* new_update){

	/*Write update TYPE*/

	fwrite( new_update,sizeof(update), 1, backup);
	fflush(backup);
}
void send_my_updates(server_variables *local_var, int min){


	if(debug){

		printf("\n Entering SEND_my_updates with min :%d\n",min);
		fflush(stdout);
	}
	LTS start,stop;
	start.LTS_counter=min;
	start.LTS_server_id=local_var->machine_id;


	int ret_val;
	node* temp=seek(local_var->update_list, start,&ret_val);
	if(ret_val==1){

		if(temp==NULL){


			temp=local_var->update_list->head->next;
		}
		else{

			temp=temp->next->next;
		}

		while(temp!=NULL){


			update* my_update = (update*)temp->data;

			if(my_update->update_lts.LTS_server_id==local_var->machine_id){

				/*This is an update in chronological order after min*/
				/*Create an update and send it */

				if(debug){
					printf("\nSending update FROM SEND_MY_UPDATE  with LTS : %d server : %d\n",my_update->update_lts.LTS_counter,my_update->update_lts.LTS_server_id);
					fflush(stdout);
				}
				send_update(local_var,my_update);


			}
			temp= temp->next;
		}



	}
	else{

		if(debug){

			printf("\n Sending EVERYTHING I have \n");
			fflush(stdout);
		}
		if(start.LTS_counter==-1){

			temp=local_var->update_list->head;

			while(temp!=NULL){

				update* my_update = (update*)temp->data;
				if(my_update->update_lts.LTS_server_id==local_var->machine_id){

					/*This is an update in chronological order after min*/
					/*Create an update and send it */

					if(debug){
						printf("\nSending update FROM SEND_MY_UPDATE  with LTS : %d server : %d\n",my_update->update_lts.LTS_counter,my_update->update_lts.LTS_server_id);
						fflush(stdout);
					}
					send_update(local_var,my_update);


				}


				temp=temp->next;
			}

		}

	}
}

void reconcile_merge(server_variables *local_var){


	int i=0,j,k;
	int min;
	int flag=1;
	if(debug){

		printf("\nEntering reconcile_merge: \n");

		printf("\nmy lts counter for min is %d\n", local_var->my_lts.LTS_counter);
		fflush(stdout);
	}


	/*First I send ALL my updates that no one in parition has*/
	/*Check to find min recvd from me*/
	int l, my_min=local_var->my_lts.LTS_counter;


	for(l=1;l<6;l++){


		if(l == local_var->machine_id)
			continue;
		if(local_var->current_members[l]==1){

			if(my_min>local_var->recvd_vectors[l][local_var->machine_id]){
				my_min=local_var->recvd_vectors[l][local_var->machine_id];

				printf("\nrecvd vector for server  %d is: %d\n", l,local_var->recvd_vectors[l][local_var->machine_id]);
			}
		}

	}


	/*Now send ALL my updates starting from my_min uptill NOW*/

	send_my_updates(local_var,my_min);

	/*current_members[i]==1 ONLY if i is present in the current partition ELSE 0
	 *
	 * j : Index of member NOT in partition (column in matrix )
	 * k : Index of member IN parition, row matrix 
	 * */
	for(j=1;j<6;j++){
		flag = 1;
		min = local_var->my_vector[j];

		if(local_var->current_members[j]!=1){

			/*Doing processing for server number j*/

			for(k=1;k<6;k++){
				if(k==local_var->machine_id || local_var->current_members[k]!=1){
					continue;
				}


				if(local_var->recvd_vectors[j][k] < min){

					min=local_var->recvd_vectors[k][j];


				}
				else if( local_var->recvd_vectors[k][j] > local_var->my_vector[j]){
					flag=0;
					break;	
				}
				else if( local_var->recvd_vectors[k][j] == local_var->my_vector[j]){

					/*Check to see if I am lower server ID*/

					if(local_var->machine_id > k){
						flag=0;
						break;
					}


				}



			}

			if(flag==1){
				LTS start,stop;

				start.LTS_counter=min;
				start.LTS_server_id=j;

				stop.LTS_counter=local_var->my_vector[j];
				stop.LTS_server_id=j;

				if(debug){
					printf("\n....going to send other peoples updates with min = %d for server %d......\n", min, j);
					fflush(stdout);

				}
				int ret_val;
				node* temp=seek(local_var->update_list, start,&ret_val);
				if(ret_val==1){

					if(temp==NULL){	
						if(debug){
							printf("\n....in recon merge temp == null for seek\n");
							fflush(stdout);
						}


						temp=local_var->update_list->head->next;
					}
					else{

						temp=temp->next->next;
					}

					while(temp!=NULL){


						update* my_update = (update*)temp->data;
						if(my_update->update_lts.LTS_counter>stop.LTS_counter){

							if(debug){
								printf("\n....found lts greater then stop...\n");
								fflush(stdout);
							}


							break;
						}
						if(my_update->update_lts.LTS_server_id==j){

							/*This is an update in chronological order after min*/
							/*Create an update and send it */

							if(debug){
								printf("\nSending update with LTS : %d server : %d\n",my_update->update_lts.LTS_counter,my_update->update_lts.LTS_server_id);
								fflush(stdout);
							}
							send_update(local_var,my_update);

						}
						temp=temp->next;
					}

				}else{


					if(debug){
						printf("\n....in min =-1 for sending other ppls stuff...\n");
						fflush(stdout);
					}

					temp= local_var->update_list->head;

					while(temp!=NULL){


						update* my_update = (update*)temp->data;
						if(my_update->update_lts.LTS_counter>stop.LTS_counter){

							if(debug){
								printf("\n....found lts greater then stop...\n");
								fflush(stdout);
							}


							break;
						}
						if(my_update->update_lts.LTS_server_id==j){

							/*This is an update in chronological order after min*/
							/*Create an update and send it */

							if(debug){
								printf("\nSending update with LTS : %d server : %d\n",my_update->update_lts.LTS_counter,my_update->update_lts.LTS_server_id);
								fflush(stdout);
							}
							send_update(local_var,my_update);

						}
						temp=temp->next;
					}


				}

			}
		}



	}

	if(debug){

		printf("\nCalling send USERS \n");
		fflush(stdout);
	}

	send_users(local_var);


	if(debug){

		printf("\nLeaving reconcile_merge: \n");
		fflush(stdout);
	}
}



void reconcile_partition(server_variables *local_var, int server_id){

	if(debug){

		printf("\nIn reconcile partition-----server_id : %d --------------\n",server_id);
		fflush(stdout);
	}

	/*Remove users from this server in my chatrooms*/
	node* tmp=local_var->server_chats[server_id]->head;
	while(tmp!=NULL){

		/*Loop through each chat room*/

		/*Get head till head == NULL (Delete each head)*/
		chatroom* my_data=(chatroom*) tmp->data;

		node* m= my_data->users->head;
		while(m!=NULL){

			meta* u= (meta*)m->data;

			response_packet *join_p1= create_response_packet(R_LEAVE,local_var);
			sprintf(join_p1->data.users[0],"%s",u->meta_user);			
			process_leave_chatroom(my_data->chatroom_name, local_var, u->meta_user, server_id );
			send_packet(my_data->chatroom_name,join_p1,local_var,0);


			m=my_data->users->head;
		}
		tmp=tmp->next;
	}


	if(debug){

		printf("\nLEAVING reconcile partition-----server_id : %d --------------\n",server_id);
		fflush(stdout);
	}

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


void send_packet(char *group_name, response_packet *packet, server_variables *local_var, int flag){


	int ret;
	/*If flag is 0 change group name to include server's ID */

	char chatroom1[SIZE]="\0";
	if(flag==0){

		strcat(chatroom1,group_name);


		strcat(chatroom1,public_server_grps[local_var->machine_id]);

	}
	else{
		strcat(chatroom1,group_name);
	}
	ret= SP_multicast( Mbox, AGREED_MESS|SELF_DISCARD,chatroom1, 1, sizeof(response_packet), (char*)packet );
	if(debug) {
		printf("\n\nPacket type is %d\n",packet->response_packet_type);
		printf("\n\nSending a response packet to %s\n\n",chatroom1);
	}

	if( ret < 0 )
	{
		SP_error( ret );
		Bye();
	}

	printf("\nLeaving send\n");
}



static  void    Assign(char *argv[])
{
	if(debug){
		//printf("\nin assign");
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

		printf("\nReturn Value :%d");

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
			printf("\nmessage from %s, of type %d,(%d bytes)\n",sender,mess_type,ret);
			fprintf(log1,"message from %s, of type %d,(%d bytes)\n",
					sender, mess_type,  ret );
		}
		if(strcmp(target_groups[0],SERVER_GRP)==0){
			printf("\nGot update");
			process_update(mess,local_var,1);
		}
		else{
			process_message(mess,sender,local_var);
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

			int i;

			if(strcmp(sender,SERVER_GRP)==0){
				int r;
				for(r=1;r<6;r++){

					local_var->current_members[r]=0;
				}

				local_var->total_members=num_groups;
				if(debug){

					printf("\nNumber of servers: %d\n",num_groups);
					fflush(stdout);
				}
				for( i=0; i < num_groups; i++ ){

					/*Extract ID from the group name*/
					int id;
					char* str=strtok( &target_groups[i][0],"#");
					char* integer=strtok(str,"s");
					id=atoi(integer);
					local_var->current_members[id]=1;

					if(debug){
						printf("\nServer with id :%d in  group",id);
						fflush(stdout);
					}



				}


			}
			//printf("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );





			if( Is_caused_join_mess( service_type ) )
			{
				printf("\nDue to join of  %s\n",  memb_info.changed_member);
				fprintf(log1,"Due to the JOIN of %s\n", memb_info.changed_member );
				printf("\nGroup joined: %s",sender);

				if(strcmp(sender,SERVER_GRP)==0){
					//init counter that keeps track of no of local vectors received
					local_var->vectors_cnt=1;

					//TODO set member ship ID here. 

					/*Reinit received vectors matrix*/
					int k,w;
					for(k=0;k<6;k++){

						for(w=0;w<6;w++)
							local_var->recvd_vectors[k][w]=0;
					}	



					//send my local vector to everyone in group
					create_merge_packet(local_var);
				}

			}else if( Is_caused_leave_mess( service_type ) ){
				printf("Due to the LEAVE of %s\n", memb_info.changed_member );
			}else if( Is_caused_disconnect_mess( service_type ) ){
				printf("Due to the DISCONNECT of %s\n", memb_info.changed_member );

				if(strcmp(sender,SERVER_GRP)==0){
					int p;
					for(p=1;p<6;p++){

						if(local_var->current_members[p]==0){

							//call reconcile partition.
							reconcile_partition(local_var,p);
						}
					}
				}

			}
			else if( Is_caused_network_mess( service_type ) ){
				printf("Due to NETWORK change with %u VS sets\n", memb_info.num_vs_sets);
				num_vs_sets = SP_get_vs_sets_info( mess, &vssets[0], MAX_VSSETS, &my_vsset_index );
				if (num_vs_sets < 0) {
					printf("BUG: membership message has more then %d vs sets. Recompile with larger MAX_VSSETS\n", MAX_VSSETS);
					SP_error( num_vs_sets );
					exit( 1 );
				}


				if(strcmp(sender,SERVER_GRP)==0){

					if(num_vs_sets==1){


						printf("\n\n*****__________Calling RECONCILE PARTITION_______****\n\n");
						/*If num_vs_sets == 1 it is a LEAVE or partition creation*/
						int p;
						for(p=1;p<6;p++){

							if(local_var->current_members[p]==0){

								//call reconcile partition.
								reconcile_partition(local_var,p);
							}

						}
					}
					else{

						//init counter that keeps track of no of local vectors received
						local_var->vectors_cnt=1;


						//TODO set member ship ID here. 

						/*Reinit received vectors matrix*/
						int k,w;
						for(k=0;k<6;k++){

							for(w=0;w<6;w++)
								local_var->recvd_vectors[k][w]=0;
						}	



						//send my local vector to everyone in group
						create_merge_packet(local_var);

					}
				}
				for( i = 0; i < num_vs_sets; i++ )
				{
					printf("%s VS set %d has %u members:\n",
							(i  == my_vsset_index) ?
							("LOCAL") : ("OTHER"), i, vssets[i].num_members );
					ret = SP_get_vs_set_members(mess, &vssets[i], members, MAX_MEMBERS);
					if (ret < 0) {
						printf("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
						SP_error( ret );
						exit( 1 );
					}
					for( j = 0; j < vssets[i].num_members; j++ )
						printf("\t%s\n", members[j] );
				}

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


void create_merge_packet(server_variables *local_var){



	if(debug){

		printf("\n  BEFORE sending MERGE: sent values : machine id %d memb id: %d    ",local_var->machine_id, local_var->current_memb_id);
		printf("\n Printing vector sent:\n");
		int i;
		for(i=0;i<6;i++){

			printf(" %d\t",local_var->my_vector[i]);
		}
		fflush(stdout);

	}

	LTS lts;
	lts.LTS_counter=-1;
	lts.LTS_server_id=-1;
	char chat[SIZE]="\0";
	int i;
	//sprintf()
	merge_packet mp;
	mp.source_id=local_var->machine_id;
	mp.memb_id=local_var->current_memb_id;
	for(i=0;i<6;i++){

		mp.vector[i]=local_var->my_vector[i];	
	}
	union_update_data merge_data;
	memcpy(&merge_data,&mp,sizeof(mp));
	node* u_merge=create_update(MERGE,lts,chat,merge_data );

	update *to_send=(update*)u_merge->data;

	send_update(local_var,to_send);

	if(debug){

		printf("\n  After sending MERGE: sent values : machine id %d memb id: %d    ",mp.source_id, mp.memb_id);
		printf("\n Printing vector sent:\n");
		for(i=0;i<6;i++){

			printf(" %d\t",mp.vector[i]);
		}
		fflush(stdout);
	}

}


void process_leave_chatroom(char* group, server_variables *local_var, char* user, int new_server){


	if(debug){

		printf("\n----------_Entering process leave chatroom with GROUP : %s USER :%s server :%d\n", group, user, new_server);
		fflush(stdout);
	}

	int ret_val;
	node *prev, *temp, *user_check;
	chatroom *my_data3;
	prev= seek_chatroom(local_var->chat_lists,group,&ret_val);
	handle_user_mappings(local_var,new_server,group,user, 0);	
	if(ret_val==1){


		if(prev==NULL){
			temp=local_var->chat_lists->head;
		}
		else{
			temp=prev->next;
		}



		my_data3=(chatroom*)temp->data;


		user_check=seek_user(my_data3->users,user,&ret_val);
		node* prev_user=user_check;
		if(ret_val==1){
			if(user_check==NULL){
				//user exists and is the first user 
				user_check=my_data3->users->head;

			}
			else{

				user_check=user_check->next;

			}
			//decrement count of user


			((meta*)user_check->data)->cnt--;

			/*If count of users equals to 0, delete this guy */
			if( (((meta*)user_check->data)->cnt) == 0){

				delete(my_data3->users,prev_user);

			}

			if(debug){
				printf("\n-------------PRINTING AFTER DELETING ------------------\n");
			}
			print_meta(my_data3->users);


		}
		else{

			printf("\n User doesn't exist in chatroom, some error in code\n");
		}



	}
	else{

		printf("\nSome error in code, chat room doesn't seem to exist! %s\n", group);
	}


	if(debug){

		printf("\n----------LEAVING  process leave chatroom with GROUP : %s USER :%s server :%d\n", group, user, new_server);
		fflush(stdout);
	}


}



void handle_user_mappings(server_variables *local_var, int new_server, char* group, char* user, int flag){


	/*If flag == 1 , we add a user - group mapping for a server. 
	 * If flag == 0, we delete a user - group mapping for a server.
	 * */
	if(debug){
		printf("\n -------Mapping for server id %d chatroom ---%s ---user %s-----flag %d-----\n",new_server,group, user, flag);
		print_chatlist(local_var->server_chats[new_server]);
		fflush(stdout);
	}

	int ret_val;
	node* prev, *prev_chatroom;
	prev= seek_chatroom(local_var->server_chats[new_server],group,&ret_val);
	prev_chatroom=prev;
	node*temp;
	/*Found chat room, if chatroom doesn't exist for this server, there's nothing to be done, in case of deleting a mapping*/
	if(ret_val==1){

		if(prev==NULL){

			temp=local_var->server_chats[new_server]->head;
		}
		else{
			temp=prev->next;
		}

		int ret_val2;
		chatroom* my_data3=(chatroom*) temp->data;
		node* prev_user;

		/*Chatroom has been found, now seek for user-chatroom mapping for this server*/
		node* user_seek= seek_user(my_data3->users,user,&ret_val2);
		prev_user=user_seek;
		if(ret_val2==1){

			if(user_seek==NULL){

				user_seek=my_data3->users->head;
			}
			else{
				user_seek=user_seek->next;
			}

			/*Operation of adding user-group mapping*/
			if(flag==1){
				((meta*)user_seek->data)->cnt++;
			}
			else{ /*Delete user group mapping*/

				/*Decrement user count*/
				((meta*)user_seek->data)->cnt--;

				/*Check if user count has reached zero, if so, delete  user node from that chat room list
				 * If linked list becomes empty
				 * Delete that chat room*/
				if( (((meta*)user_seek->data)->cnt) == 0){

					delete(my_data3->users,prev_user);

					if(is_empty(my_data3->users )){

						/*Delete that chatroom list*/
						delete(local_var->server_chats[new_server],prev_chatroom);			

					}



				}


			}
		}
		else{

			if(flag==1){
				node* user_node=create_meta(user);
				append(my_data3->users,user_node);
			}
		}

	}
	else{


		if(flag==1){
			node* newly_created_room=(node*)create_chatroom(group);
			append(local_var->server_chats[new_server],newly_created_room);

			// add user to newly created room
			/*This chatroom WILL NOT have msgs*/
			chatroom* new_room = (chatroom*)newly_created_room->data;
			node* new_user1= create_meta(user);
			append(new_room->users,new_user1);

		}
	}

	print_chatlist(local_var->server_chats[new_server]);


	if(debug){
		printf("\n -------LEAVING Mapping for server id %d chatroom ---%s ---user %s-----flag %d-----\n",new_server,group, user, flag);

		fflush(stdout);
	}

}


void send_users(server_variables *local_var){

	if(debug){


		printf("\nGoing to send list of attendees\n");
		fflush(stdout);
	}


	node* prev,*tmp;


	/*Get users from this server in my chatrooms*/
	tmp=local_var->server_chats[local_var->machine_id]->head;
	while(tmp!=NULL){

		/*Loop through each chat room*/

		/*Get head till head == NULL (Delete each head)*/
		chatroom* my_data=(chatroom*) tmp->data;

		node* m= my_data->users->head;
		while(m!=NULL){

			meta* u= (meta*)m->data;

			join_packet join_p1;
			sprintf(join_p1.join_packet_user,"%s",u->meta_user);
			node* new_update;
			union_update_data jpacket;
			memcpy(&jpacket,&(join_p1),sizeof(join_p1));
			local_var->my_lts.LTS_counter++;
			new_update=create_update(JOIN,local_var->my_lts,my_data->chatroom_name,jpacket);


			update* to_send=(update*)new_update->data;
			send_update(local_var,to_send);

			if(debug){

				printf("\nSending join for user name : %s...... chatroom : %s\n",u->meta_user,my_data->chatroom_name);
				fflush(stdout);

			}
			m=m->next;
		}
		tmp=tmp->next;
	}


}
chatroom* process_join(char* group, server_variables *local_var, char* user, int new_server){

	/*First check if this chatroom exists in the list
	 * i.e seek the list for the location of the chatroom
	 * if null is returned then the chatroom doesn't exist
	 * in that case do as below */

	int ret_val;
	node *prev, *temp, *user_check;
	chatroom *my_data3;

	if(debug){
		fprintf(log1,"\n entering process_join with chatroom name %s ...user name %s...and server id %d.\n",group, user, new_server);
		fflush(log1);
	}

	/*Handle mappings - useful on partitions*/
	handle_user_mappings(local_var, new_server, group, user,1);

	prev= seek_chatroom(local_var->chat_lists,group,&ret_val);

	//chatroom found
	if(ret_val==1){
		//chat room exists

		if(prev==NULL){
			temp=local_var->chat_lists->head;
		}
		else{
			temp=prev->next;
		}



		my_data3=(chatroom*)temp->data;

		user_check=seek_user(my_data3->users,user,&ret_val);

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

			node* user_node=create_meta(user);
			append(my_data3->users,user_node);
		}

		if(debug){
			printf("\n-------------PRINTING from join process AFTER joining------------------\n");
		}
		print_meta(my_data3->users);


	}
	else{


		node* newly_created_room=(node*)create_chatroom(group);
		append(local_var->chat_lists,newly_created_room);


		node* user_node=create_meta(user);
		my_data3=(chatroom*)newly_created_room->data;
		append(my_data3->users,user_node);

		meta* u=(meta*)my_data3->users->head->data;

		/*Join the chat group first on spread*/
		int ret;
		char gr[SIZE]="\0";
		strcat(gr,group);
		strcat(gr,public_server_grps[local_var->machine_id]);
		ret = SP_join( Mbox, gr);
		if( ret < 0 ) {
			SP_error( ret );
			Bye();
		}

	}

	if(debug){
		fprintf(log1,"\n exiting process_join with chatroom name %s ...user name %s...and returning a chat room name %s.\n",group, user, my_data3->chatroom_name);
		fflush(log1);
	}

	return my_data3;
}


int check_causality(server_variables* local_var,update* update_packet){


	if(debug){
		printf("\nEntering check causaloty\n");
		fflush(stdout);
	}
	node* temp, *prev, *line_node,*c_node;
	/*If type is of like
	 * Check seek for an LTS in messages 
	 * if LTS found - deliver it - return 1*/
	if(update_packet->update_type==LIKE || update_packet->update_type==UNLIKE){

		int ret_val;
		/*Check if chatroom exists*/
		prev=seek_chatroom(local_var->chat_lists,update_packet->update_chat_room,&ret_val);

		/*Chatroom found, search for line now*/ 
		if(ret_val==1){

			if(prev==NULL){

				prev=local_var->chat_lists->head;

			}
			else{

				prev=prev->next;
			}

			chatroom *croom= (chatroom*)prev->data;
			int ret;
			LTS line_lts=update_packet->update_data.data_like.like_packet_line_no_lts;


			node *lp1= seek(croom->chatroom_msgs,line_lts,&ret);

			if(lp1 == NULL){

				lp1=croom->chatroom_msgs->head;

			}else{

				lp1= lp1->next;
			}



			line* lp= (line*)lp1->data;
			/*Line has been found*/
			if(ret==1){

				if(update_packet->update_type==LIKE){

					if(debug){

						printf("\n Returns 1  as msg  of type like exists  \n");
					}

					return 1;
				}
				else{ 

					/*Search for users to see if user matches*/

					int ret_val2;
					print_meta(lp->line_meta);
					node* user=seek_user(lp->line_meta, update_packet->update_data.data_like.like_packet_user,&ret_val2);
					if(ret_val2==1){

						if(debug){

							printf("\n Returns 1  as msg  of type unlike and user exists  \n");
						}
						/*User in list of likes so unlike can be delivered*/
						return 1;




					}
					else{


						if(debug){

							printf("\n Returns 0  as msg is user who liked doesn't exist, so can't unlike  : user is :%s\n",update_packet->update_data.data_like.like_packet_user);
						}	
						return 0;
					}


				}


			}
			else{

				if(debug){

					printf("\n Returns 0 as line DOESN't exist \n");
				}	
				return 0;
			}

		}
		else{

			if(debug){

				printf("\nReturns 0 - CHATROOM NOT existing %s\n",update_packet->update_chat_room);
			}
			return 0;
		}
	}
	else{

		if(debug){

			printf("\n Returns 1 as msg is NOT of type LIKE / UNLIKE \n");
		}	
		return 1;
	}

	/*If type is unlike, check if like for the line exists
	 * if so, return 1 else 0*/


	if(debug){
		printf("\nLeaving check causaloty\n");
		fflush(stdout);
	}



}

/*When send_flag == 1 this update needs to be sent to my users !!*/
void process_update(char* mess, server_variables *local_var, int send_flag){

	int ret;
	update* update_packet;
	update_packet=(update*)mess;
	node *temp, *prev;

	node* new_update=create_update(update_packet->update_type, update_packet->update_lts, update_packet->update_chat_room,update_packet->update_data);

	if(update_packet->update_type!=MERGE){
		/*Do causality check*/
		int flag;

		if(send_flag == 1){
			flag= 1;//check_causality(local_var,update_packet);
		}else{

			flag =1;
		}
		//int flag=1;

		if(flag==0){
			/*Causally dependent*/


			append(local_var->undelivered_update_list,new_update);
		}
		else{
			int r;
			node* old=seek(local_var->update_list, update_packet->update_lts,&r);

			if(update_packet->update_type!=JOIN && update_packet->update_type!=LEAVE){


				insert(local_var->update_list,new_update,old);
				if(send_flag==1){
					write_to_file(local_var,update_packet);
				}
				/*Remove from undelivered*/
				/*	int ret_val10;
					node* to_del=seek(local_var->undelivered_update_list,update_packet->update_lts, &ret_val10);
					if(ret_val10==1){ //in undelivered

					if(debug){
					printf("\n Deleting to_del undelivered msg with LTS counter: %d server id: %d", update_packet->update_lts.LTS_counter,update_packet->update_lts.LTS_server_id);

					fflush(stdout);
					}
					delete(local_var->undelivered_update_list,to_del);

					}*/
			}

			/*Update my local LTS */
			if(debug){

				printf("\nGoing to update my LTS now for server: %d with counter: %d", update_packet->update_lts.LTS_server_id,update_packet->update_lts.LTS_counter);
				fflush(stdout);
			}
			local_var->my_vector[update_packet->update_lts.LTS_server_id]=update_packet->update_lts.LTS_counter;
			/*Adapt to new LTS counter*/
			if(update_packet->update_lts.LTS_counter > local_var->my_lts.LTS_counter){
				local_var->my_lts.LTS_counter=update_packet->update_lts.LTS_counter;
			}
			switch(update_packet->update_type){

				case JOIN: 
					if(debug){
						printf("\n-------------Got a MESSAge to join %s---------------",update_packet->update_chat_room);

					}
					process_join(update_packet->update_chat_room,local_var,update_packet->update_data.data_join.join_packet_user,update_packet->update_lts.LTS_server_id);
					response_packet *join_p= create_response_packet(R_JOIN,local_var);
					sprintf(join_p->data.users[0],"%s",update_packet->update_data.data_join.join_packet_user);			

					if(debug){
						printf("\n-------------Got a MESSAge to join %s---------------",update_packet->update_chat_room);

					}
					send_packet(update_packet->update_chat_room,join_p,local_var,0);

					break;
				case LIKE:
					;
					response_packet* rp=process_client_update(update_packet->update_type,local_var,update_packet->update_data.data_like.like_packet_user, update_packet->update_chat_room, update_packet->update_data.data_like.like_packet_line_no_lts);

					if(rp!=NULL && send_flag==1){

						send_packet(update_packet->update_chat_room, rp, local_var,0);
					}
					break;
				case MSG:
					if(debug){
						printf("\n In update, chatroom: %s",update_packet->update_chat_room);
					}
					line_packet lp=process_append(local_var, update_packet->update_chat_room, update_packet->update_data.data_line.line_packet_user,update_packet->update_data.data_line.line_packet_message,update_packet->update_data.data_line.line_packet_lts );

					response_packet *line_p= create_response_packet(R_MSG,local_var);
					memcpy(&(line_p->data),&(lp),sizeof(line_packet));

					if(send_flag==1)
					{
						send_packet(update_packet->update_chat_room,line_p,local_var,0);
					}


					break;
				case UNLIKE:
					;
					response_packet* rp1=process_client_update(update_packet->update_type,local_var,update_packet->update_data.data_like.like_packet_user, update_packet->update_chat_room, update_packet->update_data.data_like.like_packet_line_no_lts);
					if(rp1!=NULL && send_flag==1){

						send_packet(update_packet->update_chat_room, rp1, local_var,0);
					}

					break;
				case LEAVE:
					process_leave_chatroom(update_packet->update_chat_room,local_var,update_packet->update_data.data_join.join_packet_user,update_packet->update_lts.LTS_server_id);
					response_packet *join_p1= create_response_packet(R_LEAVE,local_var);
					sprintf(join_p1->data.users[0],"%s",update_packet->update_data.data_join.join_packet_user);			
					send_packet(update_packet->update_chat_room,join_p1,local_var,0);



					break;
			}



			/*since message has just been delivered, check if you can deliver any other msgs */
			/*			if(!is_empty(local_var->undelivered_update_list)){

						if(debug){

						printf("\nThere are undelivered MSGS, PROCESSING THEM\n");
						fflush(stdout);
						}
						node* temp=local_var->undelivered_update_list->head;
						while(temp!=NULL){
						update* old_update=(update*)temp->data;

						process_update((char*)old_update,local_var,send_flag );

						temp=temp->next;
						}


						if(debug){

						printf("\nexiting undelivered MSGS,  \n");
						fflush(stdout);
						}

						}*/

		}
	}
	else{  /*Processing merge packet*/

		/*TODO : ONLY when current memb ID`*/

		int k;
		local_var->vectors_cnt++;

		for(k=1;k<6;k++){

			local_var->recvd_vectors[update_packet->update_data.data_merge.source_id][k]=update_packet->update_data.data_merge.vector[k];
		}

		if(debug){

			printf("\n Count of local vec: %d - total members: %d \n",local_var->vectors_cnt,local_var->total_members);
			fflush(stdout);
		}
		if(local_var->vectors_cnt==local_var->total_members){

			reconcile_merge(local_var);


		}


		if(debug){

			printf("\n  after processing  MERGE: source id  %d    ",update_packet->update_data.data_merge.source_id);
			printf("\n Printing vector received:\n");
			int i;
			for(i=0;i<6;i++){

				printf(" %d\t",local_var->recvd_vectors[update_packet->update_data.data_merge.source_id][i]);
			}
			fflush(stdout);

		}


	}
}

line_packet process_append(server_variables *local_var,char *croom1, char* user, char* msg_data, LTS line_lts){

	node* prev;
	int ret_val;
	printf("\nGot msg of type append\n");
	//Here need to find out the user is part of which chat room
	//Append to that chat room
	//From message, the chatroom data is obtained
	if(debug){
		printf("\nGoing to seek chatroom %s\n",croom1);
	}
	print_chatlist(local_var->chat_lists);
	fflush(stdout);
	prev=seek_chatroom(local_var->chat_lists,croom1,&ret_val);
	printf("\n ret val --%d ",ret_val);
	chatroom *croom;
	fflush(stdout);
	line* my_data2;

	node* new_line=create_line(user,msg_data,0, line_lts);
	if(ret_val==1){


		//node* new_line=create_line(user,msg_data,0, line_lts);
		if(prev==NULL){
			croom=(chatroom*)local_var->chat_lists->head->data;
		}
		else{
			croom	= (chatroom*)prev->next->data;
		} 
		linked_list* chat_list=croom->chatroom_msgs;
		//append(chat_list,new_line);
		int ret_val3;
		node* loc=seek(chat_list, line_lts, &ret_val3);
		if(ret_val3==1){

			printf("\nLIne found, duplicating ???\n");

		}
		else{

			insert(chat_list, new_line, loc);
		}
		print_line(chat_list);
		//print_chatlist(local_var->chat_lists);
		/*Now send the same msg back to all clients in group so that they can refresh screen*/

		response_packet *line_p= create_response_packet(R_MSG,local_var);
		my_data2=(line*) new_line->data;
		memcpy(&(line_p->data),&(my_data2->line_content),sizeof(line_packet));

		//	send_packet(croom1,line_p,local_var,0);


	}
	else{


		printf("\nChatroom Doesn't exist - creating new room - HAS to be created in recover \n");



		node* newly_created_room=(node*)create_chatroom(croom1);
		append(local_var->chat_lists,newly_created_room);


		//node* user_node=create_meta(user);
		chatroom* my_data3=(chatroom*)newly_created_room->data;

		//	meta* u=(meta*)my_data3->users->head->data;


		append(my_data3->chatroom_msgs,new_line);
		/*Join the chat group first on spread*/
		int ret=9;
		char gr[SIZE]="\0";
		strcat(gr,croom1);
		strcat(gr,public_server_grps[local_var->machine_id]);
		ret = SP_join( Mbox, gr);
		if( ret < 0 ) {
			SP_error( ret );
			Bye();
		}



	}
	return (my_data2->line_content);




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

		case MSG:  			  
			local_var->my_lts.LTS_counter++;

			local_var->my_vector[local_var->machine_id]=local_var->my_lts.LTS_counter;

			if(debug){
				printf("\nI got a message, changing my vector");
				fflush(stdout);
			}

			line_packet lp=process_append(local_var,recv_packet->request_packet_chatroom, recv_packet->request_packet_user, recv_packet->request_packet_data, local_var->my_lts);


			propagate_append_update(local_var, lp, local_var->my_lts, recv_packet->request_packet_chatroom);


			response_packet *line_p= create_response_packet(R_MSG,local_var);
			memcpy(&(line_p->data),&(lp),sizeof(line_packet));

			send_packet(recv_packet->request_packet_chatroom,line_p,local_var,0);



			break;
		case UNLIKE:


			local_var->my_lts.LTS_counter++;

			local_var->my_vector[local_var->machine_id]=local_var->my_lts.LTS_counter;
			if(debug){
				printf("\nGot message::: %d type",recv_packet->request_packet_type);
			}


			response_packet *rp=process_client_update(recv_packet->request_packet_type,local_var,recv_packet->request_packet_user, recv_packet->request_packet_chatroom, recv_packet->request_packet_lts);


			propagate_like_update(local_var,recv_packet,local_var->my_lts, UNLIKE);
			if(rp!=NULL){
				send_packet(recv_packet->request_packet_chatroom,rp,local_var,0);
			}
			break;
		case JOIN: printf("\nGot a message of join");
			   printf("\nRequest to join chat room: %s",recv_packet->request_packet_data);
			   char group[SIZE]="\0";
			   sscanf(recv_packet->request_packet_data,"%s",group);
			   local_var->my_lts.LTS_counter++;


			   local_var->my_vector[local_var->machine_id]=local_var->my_lts.LTS_counter;

			   response_packet *response1=create_response_packet(R_ACK,local_var);

			   my_data3=process_join(group,local_var,recv_packet->request_packet_user,local_var->machine_id);

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

			   send_packet(sender,response1,local_var,1);


			   /*Send previous msgs*/
			   linked_list* msgs=my_data3->chatroom_msgs;
			   temp= msgs->head;


			   while(temp!=NULL){

				   response_packet *line_p= create_response_packet(R_MSG,local_var);
				   line* my_data4=(line*)temp->data;
				   //line_packet to_send;
				   memcpy(&(line_p->data),&(my_data4->line_content),sizeof(line_packet));

				   send_packet(sender,line_p,local_var,1);
				   temp= temp->next;
			   }

			   response_packet *join_p= create_response_packet(R_JOIN,local_var);
			   sprintf(join_p->data.users[0],"%s",recv_packet->request_packet_user);
			   send_packet(recv_packet->request_packet_chatroom,join_p,local_var,0);


			   /*Create a join update*/
			   propagate_join_update(local_var,recv_packet, local_var->my_lts,JOIN);

			   break;
		case HISTORY:
			   ;
			   //seek chatroom first
			   node* f_room;
			   int ret_val4;
			   node* room2= seek_chatroom(local_var->chat_lists, recv_packet->request_packet_chatroom,&ret_val4);
			   if(ret_val4==1){
				   if(room2==NULL){

					   room2=local_var->chat_lists->head;

				   }
				   else{

					   room2=room2->next;
				   }
				   chatroom* c=(chatroom*)room2->data;
				   node* t=c->chatroom_msgs->head;
				   while(t!=NULL){

					   response_packet *hp= create_response_packet(R_HISTORY,local_var);
					   line* my_datah=(line*)t->data;
					   memcpy(&(hp->data),&(my_datah->line_content),sizeof(line_packet));
					   send_packet(sender,hp,local_var,1);
					   t=t->next;
				   }
			   }
			   else{

				   if(debug){

					   printf("\nSome error, this guy doesn't have chatroom history\n");
				   }
			   }

			   break;
		case VIEW: /*Check current members in server group*/
			   ;
			   response_packet *v_response=create_response_packet(R_VIEW,local_var);
			   int m;
			   for(m=1;m<6;m++){
				   if(local_var->current_members[m]==1){
					   v_response->data.server_list[m]=m;
				   }
				   else{
					   v_response->data.server_list[m]=0;
				   }
			   }
			   send_packet(sender,v_response,local_var,1); 
			   break;
		case LIKE:


			   local_var->my_lts.LTS_counter++;

			   local_var->my_vector[local_var->machine_id]=local_var->my_lts.LTS_counter;
			   if(debug){
				   printf("\nGot message::: %d type",recv_packet->request_packet_type);
			   }



			   response_packet *rp1=process_client_update(recv_packet->request_packet_type,local_var,recv_packet->request_packet_user, recv_packet->request_packet_chatroom, recv_packet->request_packet_lts);

			   propagate_like_update(local_var,recv_packet,local_var->my_lts, LIKE);

			   if(rp1!=NULL){
				   send_packet(recv_packet->request_packet_chatroom,rp1,local_var,0);
			   }
			   /*Create an update for like*/
			   break;

		case LEAVE:

			   if(debug){
				   printf("\nGot a msg of leave\n");
				   fflush(stdout);
			   }
			   char group1[SIZE]="\0";
			   sscanf(recv_packet->request_packet_chatroom,"%s",group1);
			   if(debug){
				   printf("\n----------------_CHAT ROOM: %s--------------------",group1);
			   }
			   local_var->my_lts.LTS_counter++;


			   local_var->my_vector[local_var->machine_id]=local_var->my_lts.LTS_counter;
			   process_leave_chatroom(group1,local_var,recv_packet->request_packet_user,local_var->machine_id);
			   response_packet *leave_p= create_response_packet(R_LEAVE,local_var);
			   sprintf(leave_p->data.users[0],"%s",recv_packet->request_packet_user);
			   send_packet(recv_packet->request_packet_chatroom,leave_p,local_var,0);

			   propagate_join_update(local_var,recv_packet, local_var->my_lts,LEAVE);
			   break;

	}

}




void propagate_like_update( server_variables *local_var,request_packet* recv_packet, LTS update_lts, request var){

	like_packet lp=create_like_packet(recv_packet->request_packet_lts,recv_packet->request_packet_user);
	union_update_data lpacket;
	memcpy(&lpacket,&lp,sizeof(like_packet));
	node* new_update=create_update(var,update_lts, recv_packet->request_packet_chatroom,lpacket);

	int r;
	node* old=seek(local_var->update_list, update_lts,&r);
	insert(local_var->update_list, new_update, old);

	update* to_send=(update*)new_update->data;

	/*Write update to file*/
	write_to_file(local_var,to_send);
	send_update(local_var,to_send);


}

void propagate_join_update(server_variables *local_var,request_packet* recv_packet, LTS update_lts, request var){

	join_packet jp;
	sprintf(jp.join_packet_user,recv_packet->request_packet_user);
	node* new_update;

	union_update_data jpacket;
	memcpy(&jpacket,&jp,sizeof(jp));
	new_update=create_update(var,update_lts,recv_packet->request_packet_chatroom,jpacket);


	int r;
	node* old=seek(local_var->update_list, update_lts,&r);
	//insert(local_var->update_list, new_update, old);


	update* to_send=(update*)new_update->data;
	if(debug){
		printf("\n---------Printing propogate join message--------\n  %s \n", recv_packet->request_packet_chatroom);

	}
	send_update(local_var,to_send);

}

void propagate_append_update(server_variables *local_var,line_packet lp, LTS update_lts,char* room){


	node* new_update;
	union_update_data lpacket;
	memcpy(&lpacket,&lp,sizeof(lp));
	new_update=create_update(MSG,update_lts,room,lpacket);

	int r;
	node* old=seek(local_var->update_list, update_lts,&r);
	insert(local_var->update_list, new_update, old);




	update* to_send=(update*)new_update->data;


	/*Write update to file*/
	write_to_file(local_var,to_send);
	//print_update(local_var->update_list);

	send_update(local_var,to_send);

}

void send_update(server_variables *local_var, update* to_send){

	int ret;
	ret=SP_multicast(Mbox,AGREED_MESS|SELF_DISCARD,SERVER_GRP,1, sizeof(update),(char*)to_send);
	if(ret<0){
		SP_error(ret);
		Bye();
	}
	if(debug){

		printf("\n Sent update");
		fflush(stdout);
		printf("\nLeaving send");
	}

}


response_packet* process_client_update(request type, server_variables *local_var, char* u_user, char* u_room, LTS line_lts){



	;
	if(debug){
		printf("\nGot LIKE\n");
	}
	char group[SIZE];
	sscanf(u_room,"%s",group);
	//local_var->my_lts.LTS_counter++;
	LTS to_find;

	//got the LTS to seek
	to_find=line_lts;

	//get chatroom from msg



	//get LTS of msg that has been liked
	//

	//seek for the LTS in the chatroom that I have 
	int ret_val;
	response_packet *line_p=NULL;
	if(debug){
		printf("\nChat room to seek: %s",group);
		fflush(stdout);
	}
	node* seeked_room=seek_chatroom(local_var->chat_lists,group,&ret_val);
	if(ret_val==1){

		if(debug){
			printf("\nGot room");
		}
		if(seeked_room==NULL){ 
			seeked_room=local_var->chat_lists->head;
		}
		else{
			seeked_room=seeked_room->next;
		}

		chatroom* got_room=(chatroom*)seeked_room->data;
		int ret_val2;
		node* to_modify=seek(got_room->chatroom_msgs,to_find,&ret_val2);
		if(ret_val2==1){

			if(debug){
				printf("\nGot line");
			}
			if(to_modify==NULL){
				to_modify=got_room->chatroom_msgs->head;
			}
			else{
				to_modify=to_modify->next;
			}

			line* selected_line=(line*)to_modify->data;

			//seek for user to see if user has liked
			int ret_val3;
			node* user1=seek_user(selected_line->line_meta, u_user,&ret_val3);
			switch(type){

				case LIKE: 
					if(ret_val3!=1){
						if(debug){
							printf("\nGot user");
						}
						node* new_user= create_meta(u_user);
						append(selected_line->line_meta,new_user);
						selected_line->line_content.line_packet_likes++;

						line_p= create_response_packet(R_MSG,local_var);
						memcpy(&(line_p->data),&(selected_line->line_content),sizeof(line_packet));

						//send_packet(u_room,line_p,local_var,0);

					}

					break;

				case UNLIKE: //found something to unlike
					if(ret_val3==1){
						if(debug){
							printf("\nGot user");
						}
						delete(selected_line->line_meta,user1);
						selected_line->line_content.line_packet_likes--;
						line_p= create_response_packet(R_MSG,local_var);
						memcpy(&(line_p->data),&(selected_line->line_content),sizeof(line_packet));
						//send_packet(u_room,line_p,local_var,0);

					}

					break;
			}
		}

	}


	//Once LTS is found
	//check if the same user has liked it or not in the list of users

	//if liked, already, do nothing 
	//otherwise increment no of likes, add user to liked list


	return line_p;



}

/*This function creates a packet of the type specified with the data specified and then calls the send function on it*/
response_packet* create_response_packet(response type, server_variables* local_var){

	response_packet *new_packet=(response_packet*)malloc(sizeof(response_packet));
	new_packet->response_packet_type=type;
	return new_packet;
}


