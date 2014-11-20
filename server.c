#include<net_include.h>
#include<structures.h>


























/*Processes a user request. This  can be like/join/unlike/leave etc*/
void process_request(){


}

/*Updates LTS, creates a new update of user received actions*/
void generate_update(){


}

/*Write state to disk*/
void write_to_file(){


}

/*Refreshes data structure of screen to display chat room to user. 
 * A data structure should be passed here, this structure will be updated everytime
 * before refresh. Once updated this will be sent to client*/
void update_screen(){


}

/*Send an update to everyone in the server group
 * in the current partition*/
void send_update(){


}

/*Update is received from a server group, this is processed, LTS timestamp updated etc*/
void process_update(){

}

/*Update local LTS*/
void update_LTS(){

}

/*The following steps should be carried out on receipt of any network message.
 * Mainly consists of re-conciliation. Sending your own vector and getting vectors
 * from other servers in the partition*/
void process_network_message(){

}

/*Initialize all the data structures in memory, this is done 
 * at the time of start up or on recovery after a failure*/
void initialize_memory_data_structures(){

}


/*Called by initialize memory data structures,
 * parses on disk log to fill in the structure, how
 * many log files do we keep? Do we send a type based on what type of structure
 * we're filling in*/
void parse_log_file(){


}

