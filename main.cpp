#include "stdio.h"

#include "uv.h"

#include "unistd.h"


class tcp_worker{

    uv_tcp_t handle_;
    char read_buffer_[2000];

public:

    tcp_worker(uv_loop_t *loop_ptr){
        handle_.data=this;
        uv_tcp_init(loop_ptr, &handle_);
    }

public:

    void accept(uv_stream_t* server){
        int ret=uv_accept(server, (uv_stream_t*) &handle_);
        if (ret==0){
            uv_read_start((uv_stream_t*) &handle_, &tcp_worker::on_alloc, &tcp_worker::on_read);
        }else{
            uv_close((uv_handle_t*) &handle_, NULL);
        }
    }

private:

    static void on_alloc(uv_handle_t* handle,size_t suggested_size,uv_buf_t* buf){
        tcp_worker *worker_ptr=(tcp_worker *)(handle->data);
        buf->base=worker_ptr->read_buffer_;
        buf->len=1000;
    }

    static void work_routine(uv_work_t* req){
        sleep(2);
    }

    static void on_work_complete(uv_work_t* req, int status){
        printf("work complete --> %d\n",status);
        static uv_write_t req2;
        char *text="HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHello,World";
        uv_buf_t buf;
        buf.base=text;
        buf.len=strlen(text);
        uv_write(&req2,(uv_stream_t*)(req->data),&buf,1,&tcp_worker::on_write);
    }

    static void on_write(uv_write_t* req, int status) {
        /* Logic which handles the write result */
    }

    static void on_read(uv_stream_t* stream,ssize_t nread,const uv_buf_t* buf){
        *(buf->base+buf->len)=0;
        printf("STREAM --> %s\n",buf->base);
        uv_work_t *req_ptr=new uv_work_t();
        req_ptr->data=stream;
        uv_queue_work(stream->loop,req_ptr,&tcp_worker::work_routine,&tcp_worker::on_work_complete);
    }

};

class tcp_server{

    uv_tcp_t handle_;
    uv_loop_t *loop_ptr_;

public:

    tcp_server(uv_loop_t *loop_ptr){
        loop_ptr_=loop_ptr;
        handle_.data=this;
        uv_tcp_init(loop_ptr_,&handle_);
    }

public:

    int listen(const char* ip, int port){
        struct sockaddr_in addr;
        uv_ip4_addr(ip,port,&addr);
        uv_tcp_bind(&handle_,(const struct sockaddr *)&addr,0);
        return uv_listen((uv_stream_t *)&handle_,1000,&tcp_server::on_connection);
    }

private:

    static void on_connection(uv_stream_t* handle, int status){
        printf("new connection!\n");
        tcp_worker *data_ptr=new tcp_worker(handle->loop);
        data_ptr->accept(handle);
    }

};




int main(){

    uv_loop_t loop;

    uv_loop_init(&loop);

    tcp_server server(&loop);

    int r=server.listen("0.0.0.0", 8000);
    if (!r){
        printf("uv listen ok!\n");
    }

    uv_run(&loop,UV_RUN_DEFAULT);



    return 0;
}
