# Chat Server with Groups and Private Messages 

## Contributors
- Rishit Bhutra&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;210857
- Vedant Kale&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;211153
- Rishav Mondal &nbsp;&nbsp;&nbsp;210848

## Description
The Chat Server is a real-time communication application that allows users to connect and interact with each other through various features like instant messaging and group chats. This server-side application provides the infrastructure to facilitate seamless chatting experiences for its users.

## Features

- **User Authentication**: The server allows users to authenticate using a username and password.
- **Direct Messaging**: Send private messages to other users.
- **Broadcast Messaging**: Send a message to all connected users.
- **Group Management**: Users can create, join, or leave groups.


## Compilation

### On Windows
```bash
g++ -std=c++20 -Wall -Wextra -pedantic -pthread -o server_grp server_grp.cpp -lws2_32
```
```bash
g++ -std=c++20 -Wall -Wextra -pedantic -pthread -o client_grp client_grp.cpp -lws2_32
```
### On Linux/MacOS
```bash
g++ -std=c++20 -Wall -Wextra -pedantic -pthread -o server_grp server_grp.cpp
```
```bash
g++ -std=c++20 -Wall -Wextra -pedantic -pthread -o client_grp client_grp.cpp
```

## Usage and Commands
The Chat Server supports the following client interactions and commands:
- **Send Message**: Clients can send messages to other connected clients using the `/msg <username> <message>` command.
  - Example: `/msg john Hello, how are you?`
- **Broadcast Message**: Clients can send a message to all other connected clients using the `/broadcast <message>` command.
  - Example: `/broadcast Everyone, let's chat!`
- **Group Message**: Clients can send a message to a specific group using the `/group_msg <group_name> <message>` command.
  - Example: `/group_msg tech-team The new feature is ready for review.`
- **Create Group**: Clients can create a new group chat using the `/create_group <group_name>` command.
  - Example: `/create_group gaming-club`
- **Join Group**: Clients can join an existing group chat using the `/join_group <group_name>` command.
  - Example: `/join_group gaming-club`
- **Leave Group**: Clients can leave a group chat using the `/leave_group <group_name>` command.
  - Example: `/leave_group gaming-club`
- **Exit**: Clients can disconnect from the chat server using the `/exit` command.


## Server Flow
1. **Server Initialization:** The server initializes the required libraries, loads the user credentials from `users.txt`, and starts listening for incoming client connections.
2. **Client Authorization:** When a client connects, the server prompts for the username and password. If the credentials are correct, the client is added to the system.
3. **Client Interaction:** The client can then interact with the server using the commands listed above.
4. **Handling Multiple Clients:** Each client interaction (including commands and messages) is handled in a separate thread to allow for concurrent handling of multiple users.


## Important Functions
- `sendmess()`: Sends a message to a specific client.
- `recvmess()`: Receives a message from a client.
- `add()`: Adds a client to the system and ensures the user is not already logged in.
- `joined()`: Sends a notification to all clients when a new user joins.
- `message()`: Handles sending a private message to a user.
- `broadcast()`: Sends a message to all clients except the sender.
- `group_msg()`: Sends a message to all members of a specific group.
- `create_grp()`: Creates a new group and adds the client to it.
- `join_grp()`: Adds a client to an existing group.
- `leave_grp()`: Removes a client from a group.
- `interact_client()`: Handles the main interaction loop for a client, processing commands.
- `authorization()`: Handles the client authentication process.

## Testing

### 1. Simultaneous Login with Immediate Disconnection  
- **Objective**: Simulate 400 clients logging in and then immediately closing their connections.  
- **Execution**:  
  - All 400 clients connected to the server.  
  - After successful login, each client closed its socket instantly.  
- **Result**:  
  - The server handled all connections rapidly without noticeable delays.  
  - Execution time was minimal.  
  - No errors or crashes were observed.  

### 2. Simultaneous Login with Persistent Connections  
- **Objective**: Test how the server performs when clients remain connected indefinitely.  
- **Execution**:  
  - 400 clients logged in successfully.  
  - Instead of closing their sockets, they entered an endless loop to keep the connections alive.  
- **Result**:  
  - Execution time increased significantly.  
  - Server resources were continuously consumed.  
  - No immediate failures, but this scenario could lead to potential resource exhaustion over time.  

### 3. Multiple Clients Using the Same Credentials  
- **Objective**: Check how the authentication system handles multiple clients attempting to log in with the same username and password.  
- **Execution**:  
  - 400 clients attempted to log in using **identical credentials**.  
  - Expected behavior: Clients should receive an "Authentication Failed" message if the username is already logged in.  
- **Result**:  
  - Instead of authentication failures, the server returned **unexpected errors**.  
  - Reviewing the code suggested that multiple logins for the same account **should not** be allowed, yet the error indicated a flaw in the authentication mechanism.  

### Observations and Potential Issues  
- The server efficiently handles a large number of clients logging in and disconnecting rapidly.  
- When connections remain open indefinitely, execution time increases significantly, suggesting potential resource management concerns.  
- The authentication system does not behave as expected when multiple clients attempt to log in with the same credentials. Further debugging is required to pinpoint the issue. 

## Configuration
The chat server can be configured with the following parameters:
- `host`: The hostname or IP address of the chat server.
- `port`: The port number on which the chat server is listening for incoming connections.
- `max_clients`: The maximum number of concurrent clients the server can handle.
- `message_buffer_size`: The maximum size of the message buffer for each client.
