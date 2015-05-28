/*
 * Copyright (c) 2015, Shinya Yagyu
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its 
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <assert.h>
#include "Storjutp.h"

#define TIMEOUT 500
#define BUFSIZE 65536


#define LOG(fmt, ...) log(__FILE__, __LINE__, __func__, fmt, ## __VA_ARGS__)
void *log(const char *file, int line, const char *function,
                        const char * format, ...) {
  char buffer[256];
  va_list args;
  va_start (args, format);
  vsnprintf (buffer, 256, format, args);
  fprintf(stderr,"%s:%d %s() %s\n", file, line, function, buffer);
  fflush(stderr);
  va_end (args);
  return NULL;
}

string byte2string(unsigned char *bytes, string& result){
    for(int i=0;i<32;i++){
        char s[3];
        sprintf(s, "%02X",bytes[i]);
        result += s;
    }
    return result;
}

ReceiveFileInfo *checkHash(UnknownFileInfo *fi, Storjutp *sutp, 
                           utp_socket *socket){
    ReceiveFileInfo *fii = sutp->findFileInfo(fi->hash);
    LOG("unknown file size=%ld", fi->size);
    if(!fii){
     	LOG("rejected");
        utp_set_userdata(socket, NULL);
        utp_close(socket);
        delete fi;
        return NULL;
    }
  	LOG("newly accepted");
    fii->size = fi->size;
    utp_set_userdata(socket, fii);
    sutp->fileInfos.remove(fii);
    delete fi;
    return fii;
}

uint64 callback_on_read(utp_callback_arguments *a){
    FileInfo *_fi= (FileInfo *)utp_get_userdata(a->socket);
    Storjutp *sutp = (Storjutp *) utp_context_get_userdata(a->context);

   	size_t left = a->len;
    const byte *p = a->buf;

    if(!_fi){
        _fi = new UnknownFileInfo();
        utp_set_userdata(a->socket, _fi);
    }
    
    UnknownFileInfo *ufi = dynamic_cast<UnknownFileInfo *>(_fi);
    if(ufi){
        size_t r = ufi->putByte(p, left);
	    left -= r;
        p+=r;
        if(ufi->hasHash()){
            _fi = checkHash(ufi, sutp, a->socket);
        }
    }
        
    ReceiveFileInfo *rfi = dynamic_cast<ReceiveFileInfo *>(_fi);
    if(rfi){
        size_t len = 0;
    	for(len = 0;left; left -=len, p+=len) {
	    	len = fwrite(p, 1, left, rfi->fp);
    	}
    }
    if(left==0){
    	utp_read_drained(a->socket);
    }
	return 0;
}

uint64 callback_on_error(utp_callback_arguments *a){
	LOG("Error: %s", utp_error_code_names[a->error_code]);
    FileInfo *fi = (FileInfo *)utp_get_userdata(a->socket);
    Storjutp *sutp = (Storjutp *) utp_context_get_userdata(a->context);

    if(fi && fi->handler){
        LOG("calling handler %x",fi->handler);
        fi->handler->on_finish(fi->hash,
                               utp_error_code_names[a->error_code]);
    }
    if(fi){
        sutp->deleteFileInfo(fi);
        utp_set_userdata(a->socket, NULL);
        utp_close(a->socket);
    }
    return 0;
}

uint64 callback_on_accept(utp_callback_arguments *a){
   	LOG("on_accept");
    //from server to client
    // do return 0, or not accepted.
	return 0;
}

void sendBytes(utp_socket *s, SendFileInfo *fi, Storjutp *sutp){
    size_t len = 0, len_u = 0;
    unsigned char buf[BUFSIZE];
    do {
        len_u = 0;
        len = fi->getByte(buf, 40);
        if(len > 0){
            len_u = utp_write(s, buf, len);
            if(len_u < len){
                fi->seek(len_u - len);
            }
        }
    }while(len_u > 0);
    
    do {
        len_u = 0;
        len = fread(buf, 1, BUFSIZE, fi->getFP());
        if(len > 0){
            len_u = utp_write(s, buf, len);
            if(len_u < len){
                fseek(fi->getFP(), len_u - len, SEEK_CUR);
            }
        }
    }while(len_u > 0);

    if(fi->isCompleted()){
        fi->handler->on_finish(fi->hash, NULL);
        sutp->deleteFileInfo(fi);
        utp_set_userdata(s, NULL);
        utp_close(s);
    }
}

uint64 callback_on_state_change(utp_callback_arguments *a){
    FileInfo *_fi = (FileInfo *)utp_get_userdata(a->socket);
    Storjutp *sutp = (Storjutp *) utp_context_get_userdata(a->context);

	switch (a->state) {
		case UTP_STATE_CONNECT:
            //fall through
		case UTP_STATE_WRITABLE:
        //from client to server
        {
            SendFileInfo *ufi = dynamic_cast<SendFileInfo *>(_fi);
            if(ufi){
	            sendBytes(a->socket, ufi, sutp);
            }
            break;
        }
        //called when utp_closed from the counterparty.
		case UTP_STATE_EOF:
        {
        	LOG("state_eof fd=%d",sutp->fd);
            if(_fi && _fi->handler){
                const char *reason = NULL;
                if(!_fi->isCompleted()){
                    reason = "disconnected from peer";
                }
                _fi->handler->on_finish(_fi->hash, reason);
            }
            utp_set_userdata(a->socket, NULL);
            sutp->deleteFileInfo(_fi);
            utp_close(a->socket);
			break;
         }
        //called from destructor of utp_socket.
		case UTP_STATE_DESTROYING:
        {
          	LOG("socket is destyoed fd=%d",sutp->fd);
            if(_fi){
                sutp->deleteFileInfo(_fi);
            }
			break;
        }
    }

	return 0;
}


uint64 callback_sendto(utp_callback_arguments *a){
    Storjutp *sutp = (Storjutp *) utp_context_get_userdata(a->context);
    sendto(sutp->fd, a->buf, a->len, 0,
           a->address, a->address_len);
	return 0;
}

struct addrinfo* getAddrInfo(string addr, int port){
    struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
    
    int error = 0;
    char p[10];
    sprintf(p, "%d", port);
    if ((error = getaddrinfo(addr.c_str(), p, &hints, &res)))
        return NULL;
	
    return res;
}

void Storjutp::deleteFileInfo(FileInfo *fi){
    ReceiveFileInfo *rfi = dynamic_cast<ReceiveFileInfo *>(fi);

    if(rfi){
        fileInfos.remove(rfi);
    }
    delete fi;
}

Storjutp::Storjutp(int port) {
    ctx = utp_init(2);
  	utp_set_callback(ctx, UTP_ON_READ, &callback_on_read);
  	utp_set_callback(ctx, UTP_ON_ACCEPT, &callback_on_accept);
    utp_set_callback(ctx, UTP_ON_ERROR, &callback_on_error);
    utp_set_callback(ctx, UTP_ON_STATE_CHANGE,&callback_on_state_change);
  	utp_set_callback(ctx, UTP_SENDTO, &callback_sendto);

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    utp_context_set_userdata(ctx, this);
    for(server_port = port; server_port < 65535; server_port++){
        struct addrinfo *res = getAddrInfo("0.0.0.0", server_port);
	    if(res  && !bind(fd, res->ai_addr, res->ai_addrlen)){
            freeaddrinfo(res);
            break;
        }
     	freeaddrinfo(res);
    }
    stopFlag = 0;
}


int Storjutp::registHash(unsigned char* hash, Handler *handler){
    if(findFileInfo(hash)) return -1;
    ReceiveFileInfo *fi = new ReceiveFileInfo();
    memcpy(fi->hash, hash, 32);
    string fname;
    byte2string(hash, fname);
    fi->fp=fopen(fname.c_str(), "wb");
    fi->handler = handler;
    fileInfos.push_back(fi);
    LOG("regist %s at fd=%d",fname.c_str(), fd);
    return 0;
}

ReceiveFileInfo *Storjutp::findFileInfo(unsigned char* hash){
    list<ReceiveFileInfo *>::iterator itr;
    bool match = false;
    for(itr = fileInfos.begin();itr != fileInfos.end();itr++){
        if((*itr)->equal(hash)){
            match=true;
            break;
        }
    }
    if(match){
        return *itr;
	}
    return NULL;
}

void Storjutp::unregistHash(unsigned char* hash){
    ReceiveFileInfo *fi = findFileInfo(hash);
    if(fi){
        deleteFileInfo(fi);
    }
}

int Storjutp::sendFile(string dest, int port, string fname, 
                             unsigned char* hash, Handler *handler){
    FILE *fp = fopen(fname.c_str(), "rb");
    if(!fp) return -1;
    struct addrinfo *res = getAddrInfo(dest, port);
    if(!res) return -1;
    SendFileInfo *f= new SendFileInfo();
    f->setFP(fp);
    LOG("sending %s at fd=%d",fname.c_str(), fd);
    utp_socket *socket = utp_create_socket(ctx);
    utp_connect(socket, res->ai_addr, res->ai_addrlen);
  	freeaddrinfo(res);
    utp_set_userdata(socket, f);
    memcpy(f->hash, hash, 32);
    f->handler = handler;
    return 0;
}

void Storjutp::setStopFlag(int flag){
    stopFlag=flag;
}

void Storjutp::start(){
    LOG("running fd=%d", fd);
    unsigned char socket_data[BUFSIZE];
   	struct sockaddr_in src_addr;
   	socklen_t addrlen = sizeof(src_addr);
    struct pollfd fds[1];
  
    while(!stopFlag) {
        fds[0].fd = fd;
        fds[0].events = POLLIN | POLLERR;
        poll(fds, 1, TIMEOUT);
        if (fds[0].revents & POLLIN) {
            int len = 0;
            do{
                len = recvfrom(fd, socket_data, BUFSIZE, MSG_DONTWAIT, 
                               (struct sockaddr *)&src_addr, &addrlen);
                if (len < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        utp_issue_deferred_acks(ctx);
                    }
                }else{
                    utp_process_udp(ctx, socket_data, len, 
                                    (struct sockaddr *)&src_addr, addrlen);
                }
            }while(len>0);
        }
      	utp_check_timeouts(ctx);
    }
}        
    
Storjutp::~Storjutp() {
    LOG("destructing fd=%d %d", fd, fileInfos.size());
    list<ReceiveFileInfo *>::iterator itr;
    for( itr = fileInfos.begin(); itr != fileInfos.end(); itr++ ){
        //don't use deleteFileInfo(), must not erase
        delete *itr;
    }
 	if(ctx) utp_destroy(ctx);
}
