/*
 *
 * CSEE 4840 Lab 2 for 2022
 *
 * Name/UNI: Elysia Witham (ew2632)
 */
#include "fbputchar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>

/* Update SERVER_HOST to be the IP address of
 * the chat server you are connecting to
 */
/* arthur.cs.columbia.edu */
#define SERVER_HOST "128.59.19.114"
#define SERVER_PORT 42000

#define BUFFER_SIZE 128

#define ENTER 0x28
#define BACKSPACE 0x2A

#define LEFT_SHIFT 0xE1
#define RIGHT_SHIFT 0xE5

#define LEFT 0x50
#define RIGHT 0x4F

/*
 * References:
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;
void *network_thread_f(void *);

char convert_ascii(int mod, int keycode) {
  if (keycode > 56) {
    return '\0';
  }
  char table[53] = {'a', 'b', 'c', 'd', 'e', 'f', 
                     'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 
                     'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2', 
                     '3', '4', '5', '6', '7', '8', '9', '0', '\n', '\0', '\0', 
                     '\0', ' ', '-', '=', '[', ']', '\\', '\0', ';', '\'', '`', 
                     ',', '.', '/'};
  char shift_table[53] = {'A', 'B', 'C', 'D', 'E',
                          'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
                          'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
                          'Z', '!', '@', '#', '$', '%', '^', '&', '*', '(', 
                          ')', '\n', '\0', '\0', '\0', ' ', '_', '+', '{', 
                          '}', '|', '\0', ':', '"', '~', '<', '>', '?'};
  int shift = 0b00100010;
  if (mod & shift) {
    return shift_table[keycode - 4];
  }
  return table[keycode - 4];
}

int main()
{
  int err, col;

  struct sockaddr_in serv_addr;

  struct usb_keyboard_packet packet;
  int transferred;
  char ascii_char;

  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }

  for (int col = 0; col < 64; col++) {
    for (int row = 0; row < 24; row++) {
      fbputchar(' ', row, col);
    }
  }

  /* Separate send/receive */
  for (col = 0 ; col < 64 ; col++) {
    fbputchar('-', 21, col);
  }

  /* Open the keyboard */
  if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
    fprintf(stderr, "Did not find a keyboard\n");
    exit(1);
  }
    
  /* Create a TCP communications socket */
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    fprintf(stderr, "Error: Could not create socket\n");
    exit(1);
  }

  /* Get the server address */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);
  if ( inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Could not convert host IP \"%s\"\n", SERVER_HOST);
    exit(1);
  }

  /* Connect the socket to the server */
  if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
    exit(1);
  }

  /* Start the network thread */
  pthread_create(&network_thread, NULL, network_thread_f, NULL);

  /* Look for and handle keypresses */
  int current_row = 22;
  int current_col = 0;
  char buf[BUFFER_SIZE];
  int pos = 0;
  int r;
  int endline = 0;
  int last_key;
  for (;;) {
    libusb_interrupt_transfer(keyboard, endpoint_address,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);
    if (transferred == sizeof(packet)) {
      if (packet.keycode[0] == 0x29) { /* ESC pressed? */
	break;
      }
      else if (packet.keycode[0] == 0) continue;
      else if (packet.keycode[0] == BACKSPACE) {
        if (current_row == 22 && current_col == 0) continue;
        fbputchar(' ', current_row, current_col);
        buf[pos] = '\0';
        if (current_col == 0) {
          current_col = 63;
          current_row--;
        }
        else {
          current_col--;
        }
        pos--;
        endline--;
        last_key = packet.keycode[0];
      }
      else if (packet.keycode[0] == ENTER) {
        if (strlen(buf) == 0) continue;
        buf[endline] = '\n';
        buf[endline+1] = '\0';
        if ((r = write(sockfd, buf, endline+1)) < 0 ) {
          fprintf(stderr, "Error: Write to socket failed\n");
          exit(1);
        }
        for (int x = 0; x < BUFFER_SIZE; x++) {
          buf[x] = '\0';
        }
        for (current_col = 0; current_col < 64; current_col++) {
          for (current_row = 22; current_row < 24; current_row++) {
            fbputchar(' ', current_row, current_col);
          }
        }
        current_row = 22;
        current_col = 0;
        pos = 0;
        endline = 0;
        last_key = packet.keycode[0];
      }
      else if (packet.keycode[0] == LEFT) {
        if (current_row == 22 && current_col == 0) continue;
        if (pos != endline)
          fbputchar(buf[pos], current_row, current_col);
        else 
          fbputchar(' ', current_row, current_col);
        if (current_col == 0) {
          current_col = 63;
          current_row--;
        }
        else {
          current_col--;
        }
        pos--;
        last_key = packet.keycode[0];
      }
      else if (packet.keycode[0] == RIGHT) {
        printf("%d", endline);
        if (pos == endline || buf[pos] == '\0') continue;
        fbputchar(buf[pos], current_row, current_col);
        if (current_col == 64) {
          current_col = 0;
          current_row++;
        }
        else {
          current_col++;
        }
        pos++;
        last_key = packet.keycode[0];
      }
      else if (current_col == 62 && current_row == 23) continue;
      else {
        for (int i = 0; i < 6; i++) {
          if (!packet.keycode[i]) {
            if (packet.keycode[i-1] == 0 || packet.keycode[i-1] == last_key) {
              last_key = 0;
              continue;
            }
            ascii_char = convert_ascii(packet.modifiers, packet.keycode[i-1]);
            last_key = packet.keycode[i-1];
            break;
          }
        }
        if (ascii_char == '\0' || ascii_char == 0) continue;
        buf[pos] = ascii_char;
        fbputchar(ascii_char, current_row, current_col);
        current_col++;
        if (current_col > 63) {
          current_col = 0;
          current_row++;
        }
        if (pos == endline) endline++;
        pos++;
      }
      fbputchar('|', current_row, current_col);
      // sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0], packet.keycode[1]);
    }
  }

  /* Terminate the network thread */
  pthread_cancel(network_thread);

  /* Wait for the network thread to finish */
  pthread_join(network_thread, NULL);

  return 0;
}

void *network_thread_f(void *ignored)
{
  char recvBuf[BUFFER_SIZE];
  int n;
  int recv_row;
  /* Receive data */
  while ( (n = read(sockfd, &recvBuf, BUFFER_SIZE - 1)) > 0 ) {
    recvBuf[n] = '\0';
    printf("%s", recvBuf);
    if (recv_row > 19) {
      for (int recv_col = 0; recv_col < 64; recv_col++) {
        for (recv_row = 0; recv_row < 21; recv_row++) {
          fbputchar(' ', recv_row, recv_col);
        }
      }
      recv_row = 0;
    }
    if (n > 64) {
      for (int m = 0; m < 2; m++) {
        for (int k = 0; k < 64; k++) {
          if (k + (m*64) >= (n-1)) break;
          fbputchar(recvBuf[k + (m*64)], recv_row + m, k);
        }
      }
      recv_row+=2;
    }
    else {
      for (int x = 0; x < (n-1); x++)
        fbputchar(recvBuf[x], recv_row, x);
      recv_row++;
    }
  }

  return NULL;
}

