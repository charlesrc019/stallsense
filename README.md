# StallSense Web Server

## Overview
The StallSense Web Server provides real-time updates for bathroom stall occupancy using MQTT and WebSocket technologies. It integrates with a set of thermal and door sensors to monitor the status of stalls, allowing users to see live data about bathroom occupancy.

## Key Components
- **Express.js**: Used for server-side routing and middleware management.
- **Socket.io**: Enables real-time communication with clients, pushing updates when stall statuses change.
- **Sequelize ORM**: Interacts with the database to store and retrieve sensor data, ensuring the latest stall state is reflected on the user interface.
- **MQTT**: Handles communication between sensors and the server, subscribing to topics and processing incoming messages related to stall occupancy.

## Functionality

- **MQTT Communication**:
  - Subscribes to all topics (`#`), receiving messages from connected sensors that indicate whether a stall is occupied or vacant.
  - Processes incoming messages (`0` or `1`), updating the sensor's status in the database and emitting status changes to connected clients via WebSocket.
  - Handles special topics for sensor identification and IP address updates, ensuring the system is always aware of the connected devices.

- **WebSocket Integration**:
  - Sends real-time occupancy data to clients when a sensor's state changes, allowing users to view updates instantly without refreshing the page.

- **Database Interaction**:
  - Uses Sequelize to query and update sensor data in the database. When a sensor sends a state change, the corresponding entry in the database is updated, and the change is reflected in real-time to all users via WebSocket.

## Setup

1. **Install Dependencies**:
   - `npm install express http-errors morgan mqtt socket.io sequelize`
   - Install any required database libraries and configure Sequelize.

2. **Run the Server**:
   - Start the server using `npm start`.
   - Ensure MQTT broker is running locally on `mqtt://127.0.0.1`.

3. **Connect Sensors**:
   - Ensure sensors are sending MQTT messages with appropriate topics and payloads for occupancy states, identification, and IP updates.

## Future Enhancements
- Add support for more sensor types.
- Implement user authentication for access control.
- Extend functionality to handle more complex sensor setups and data types.
