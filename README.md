Serial-to-Ethernet-Adapter
==========================

An arduino based board to convert an ENC28J60 ethernet module into an adapter with a serial port.

When using the cheap and commonly available ENC28J60 ethernet module with an arduino you typically need 10 to 20kb worth of library code, this can cause problems if you need the program space for other applications.

This adapter once configured takes incomming serial data and POST's it to a url, the response is then parsed and returned on the serial line.
This makes it incredibly easy to upload data to a server or request information as all it takes is a serial command which is typically available by default.

The device is configured using a special serial command which sets the host, url and connection method, get or post.


##Usage

### Setup
The setup command is prefixed with a plus (+) sign, if this apears at the start of a line then that line is assumed to be a setup string.

    +www.example.com:/url-to-send-data-to:1
   
The string takes 3 parameters which are seperated with colons (:).

    +HOST:URL:FORMAT
   
The HOST must not include the http:// part and needs to be http port 80 only.<br />
The URL should begin with a forward slash and include the rest of the url.<br />
The FORMAT can be one of 3 integers

- 0: GET
- 1: POST
- 2: POST (body)

The GET request doesn't send any data and is usefull for requesting information, just send a blank line to the adapter and the response will be returned.

The first POST option sends the incomming data line to the url as a form post with everything contained under the parameter "data".

The second POST option sends the incomming data line in the body with the content type set to application/json, ideal for sending basic json encoded data.

###Using It
To use the adapter all you need to do is send a single line to the serial port, it will send this to the server and relay the response back to you.
It only supports single line requests so make sure the only line break character is ending the message.

###Status Outputs
The board has 2 status outputs, a busy line and a ready line.

The Ready line is used during setup and will be low untill the board has loaded and operational, at which point it will remain high untill the device gets reset.

The Busy line goes high every time a request comes in and it starts to do something. Its a good idea to monitor this and not send anything while its high.

###Reset Input
The latest version of the board makes available the ATMEGA's reset pin, this can be usefull to tie into to ensure a clean start or if it starts misbehaving.

###To Do
The code could do with tidying up in places and there are a number of fixed options I would like to make configurable.
