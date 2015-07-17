/*
Copyright (c) 2015 Mike Brady

A text-only sample metadata player for Shairport Sync

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/



#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>


// From Stack Overflow, with thanks:
// http://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
// minor mods to make independent of C99.
// more significant changes make it not malloc memory
// needs to initialise the docoding table first

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static int decoding_table[64];

static int mod_table[] = {0, 2, 1};

void initialise_decoding_table() {
    int i;
    for (i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}

// pass in a pointer to the data, its length, a pointer to the output buffer and a pointer to an int containing its maximum length
// the actual length will be returned.

char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    char *encoded_data,
                    size_t *output_length) {

    size_t calculated_output_length = 4 * ((input_length + 2) / 3);    
    if (calculated_output_length> *output_length)
      return(NULL);
    *output_length = calculated_output_length;
    
    int i,j;
    for (i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}


// pass in a pointer to the data, its length, a pointer to the output buffer and a pointer to an int containing its maximum length
// the actual length will be returned.
int base64_decode(const char *data,
                             size_t input_length,
                             unsigned char *decoded_data,
                             size_t *output_length) {

    //remember somewhere to call initialise_decoding_table();

    if (input_length % 4 != 0) return -1;

    size_t calculated_output_length = input_length / 4 * 3; 
    if (data[input_length - 1] == '=') calculated_output_length--;
    if (data[input_length - 2] == '=') calculated_output_length--;
    if (calculated_output_length> *output_length)
      return(-1);
    *output_length = calculated_output_length;
 
    int i,j;
    for (i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return 0;
}

int main(void) {
  fd_set rfds;
  int retval;
  
  initialise_decoding_table();

  mkdir("/tmp/shairport-sync", 0755);

  /* Watch stdin (fd 0) to see when it has input. */
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  
  while (1) {
    int tag_found = 0;
    char str[1024];
    while (tag_found==0) {
      retval = select(1, &rfds, NULL, NULL, NULL);
      /* Don’t rely on the value of tv now! */
      if (retval == -1)
        perror("select()");
      else if (retval) {
        char *rp;
        rp = fgets (str, 1024, stdin);
        if (rp!=NULL) {
          if (str[0]=='<') {
          tag_found=1;// now check if the first character is '<'
//          } else {
//            printf(".\n");
          }
        }
        /* FD_ISSET(0, &rfds) will be true. */
      } else
        printf("No data.\n");
    }
    // now, parse it to see if it's a tag
    uint32_t type,code,length;
    char tagend[1024];
    int ret = sscanf(str,"<item><type>%8x</type><code>%8x</code><length>%u</length>",&type,&code,&length);
    if (ret==3) {
      // now, think about processing the tag. 
      // basically, we need to get hold of the base-64 data, if any
      size_t outputlength=0;
      char payload[32769];
      if (length>0) {
        // get the next line, which should be a data tag
        char datatagstart[64],datatagend[64];
        memset(datatagstart,0,64);
        int rc = fscanf(stdin,"<data encoding=\"base64\">");
        if (rc==0) {
          // now, read in that big (possibly) base64 buffer
          int c = fgetc(stdin);
          uint32_t b64size = 4*((length+2)/3);
          char * b64buf = malloc(b64size+1);
          memset(b64buf,0,b64size+1);
          if (b64buf) {
            if (fgets(b64buf, b64size+1, stdin)!=NULL) {
              // it looks like we got it
              // printf("Looks like we got it, with a buffer size of %u.\n",b64size);
              //puts(b64buf);
              //printf("\n");
              // now, if it's not a picture, let's try to decode it.
              if (code!='PICT') {
                int inputlength=32678;
                if (b64size<inputlength)
                  inputlength=b64size;
                  outputlength=32768;
                if (base64_decode(b64buf,inputlength,payload,&outputlength)!=0) {
                  printf("Failed to decode it.\n");
                }
              } else {
                FILE *f = fopen("/tmp/shairport-sync/image", "w");
                if (f == NULL)
                {
                  printf("Can't open the file to write!\n");
                  exit(1);
                }
                fprintf(f, "%s", b64buf);
                fclose(f);
              }
            }
            free(b64buf);
          } else {
            printf("couldn't allocate memory for base-64 stuff\n");
          }
          rc = fscanf(stdin,"%64s",datatagend);
          if (strcmp(datatagend,"</data></item>")!=0) 
            printf("End data tag not seen, \"%s\" seen instead.\n",datatagend);
        }
      } 
     
      // printf("Got it decoded. Length of decoded string is %u bytes.\n",outputlength);
      payload[outputlength]=0;

      // this has more information about tags, which might be relevant:
      // https://code.google.com/p/ytrack/wiki/DMAP
      switch (code) {
        case 'asal':
          printf("Album Name: \"%s\".\n",payload);
          FILE *g = fopen("/tmp/shairport-sync/album", "w");
          fprintf(g, "%s", payload);
          fclose(g);
          break;
        case 'asar':
          printf("Artist: \"%s\".\n",payload);
          FILE *h = fopen("/tmp/shairport-sync/artist", "w");
          fprintf(h, "%s", payload);
          fclose(h);
          break;
        case 'ascm':
          printf("Comment: \"%s\".\n",payload);
          FILE *i = fopen("/tmp/shairport-sync/comment", "w");
          fprintf(i, "%s", payload);
          fclose(i);
          break;
        case 'asgn':
          printf("Genre: \"%s\".\n",payload);
          FILE *j = fopen("/tmp/shairport-sync/genre", "w");
          fprintf(j, "%s", payload);
          fclose(j);
          break;
        case 'minm':
          printf("Title: \"%s\".\n",payload);
          FILE *k = fopen("/tmp/shairport-sync/title", "w");
          fprintf(k, "%s", payload);
          fclose(k);
          break;
        case 'ascp':
          printf("Composer: \"%s\".\n",payload);
          FILE *l = fopen("/tmp/shairport-sync/composer", "w");
          fprintf(l, "%s", payload);
          fclose(l);
          break;                    
        case 'asdt':
          printf("File kind: \"%s\".\n",payload);
          FILE *m = fopen("/tmp/shairport-sync/kind", "w");
          fprintf(m, "%s", payload);
          fclose(m);
          break;  
        case 'assn':                
          printf("Sort as: \"%s\".\n",payload);
          break;
        case 'PICT':
          printf("Picture received, length %u bytes.\n",length);
          break;               
        default: if (type=='ssnc') {
            char typestring[5];
            *(uint32_t*)typestring = htonl(type);
            typestring[4]=0;
            char codestring[5];
            *(uint32_t*)codestring = htonl(code);
            codestring[4]=0;
            printf("\"%s\" \"%s\": \"%s\".\n",typestring,codestring,payload);
            if (code=='snam') {
              FILE *n = fopen("/tmp/shairport-sync/sender", "w");
              fprintf(n, "%s", payload);
              fclose(n);
            }
            if (code=='snua') {
              FILE *n = fopen("/tmp/shairport-sync/useragent", "w");
              fprintf(n, "%s", payload);
              fclose(n);
            }
            if (code=='pend') {
              DIR *outputDir = opendir("/tmp/shairport-sync");
              struct dirent *next_file;
              char filepath[256];

              while ( (next_file = readdir(outputDir)) != NULL ) {
                // build the path for each file in the folder
                sprintf(filepath, "%s/%s", "/tmp/shairport-sync", next_file->d_name);
                remove(filepath);
              }
              
            }
          }
       }
     } 
   }
  return 0;
}
