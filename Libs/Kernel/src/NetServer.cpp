/*-----------------------------------------------------------------------------
Copyright(c) 2010 - 2018 ViSUS L.L.C.,
Scientific Computing and Imaging Institute of the University of Utah

ViSUS L.L.C., 50 W.Broadway, Ste. 300, 84101 - 2044 Salt Lake City, UT
University of Utah, 72 S Central Campus Dr, Room 3750, 84112 Salt Lake City, UT

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

For additional information about this project contact : pascucci@acm.org
For support : support@visus.net
-----------------------------------------------------------------------------*/

#include <Visus/NetServer.h>
#include <Visus/VisusConfig.h>
#include <Visus/ApplicationInfo.h>
#include <Visus/Log.h>


namespace Visus {

///////////////////////////////////////////////////////
class HttpNetServer : public NetServer::Pimpl
{
public:

  int                        port;
  int                        verbose;
  SharedPtr<NetServerModule> module;
  SharedPtr<std::thread>     thread;
  bool                       bExitThread=false;
  
  //constructor
  HttpNetServer(int port_) : port(port_)
  {
    this->verbose = cint(VisusConfig::readString("Configuration/NetServer/verbose",ApplicationInfo::debug?"1":"0"));
  }

  //destructor
  ~HttpNetServer()
  {
    if (thread && thread->joinable())
    {
      //could be that the thread is stuck in server->accept
      auto client=std::make_shared<NetSocket>();
      client->connect("http://127.0.0.1:"+cstring(port));
      bExitThread=true;
      Thread::join(thread);
    }
  }

  //addModule
  virtual void addModule(SharedPtr<NetServerModule> value) override
  {
    VisusAssert(!this->module);
    this->module=value;
  }

  //runInThisThread
  virtual void runInThisThread() override
  {
    VisusAssert(this->module);
    this->entryProc();
  
  }

  //runInBackground
  virtual void runInBackground() override
  {
    VisusAssert(this->module);
    this->thread=Thread::start("HttpNetServer Thread",[this](){
      entryProc();
    });
  }

  //writeResponse
  bool writeResponse(NetSocket* client,NetResponse response)
  {
    response.setHeader("Connection","Close");//in this debug version I don't keep the connections alive!
    response.setHeader("NetServer","Visus debugging server");//just as double check
    response.setHeader("Access-Control-Allow-Origin","*");//accept connections from localhost
    client->sendResponse(response);
    client->shutdownSend();
    return true;
  }

  //entryProc
  void entryProc() 
  {
    String url="http://127.0.0.1:"+cstring(port);

    auto server=std::make_shared<NetSocket>();
    if (!server->bind(url))
    {
      VisusError() << "NetServer::entryProc bind on port("<<port<<") failed";
      return;
    }

    auto thread_pool=std::make_shared<ThreadPool>("HttpServer Worker",64);

    //loop accept connections/handle operation
    while (!bExitThread)
    {
      if (auto client=server->acceptConnection())
      {
        thread_pool->asyncRun([this,client](int worker)
        {
          if (bExitThread)
          {
            //maybe the client closed the connection
            writeResponse(client.get(), NetResponse(HttpStatus::STATUS_INTERNAL_SERVER_ERROR));
          }
          else
          {
            NetRequest request = client->receiveRequest();
            if (!request.valid())
            {
              writeResponse(client.get(), NetResponse(HttpStatus::STATUS_BAD_REQUEST));
            }
            else
            {
              NetResponse response = module->handleRequest(request);
              bool bWrote = writeResponse(client.get(), response);
              if (verbose)
              {
                if (response.isSuccessful())
                {
                  if (!bWrote)
                    VisusInfo() << "Error writing the netresponse to the client, maybe he just dropped the request?";
                  else
                    VisusInfo() << "Wrote netresponse to the client";
                }
                else
                {
                  if (verbose)
                    VisusInfo() << "!response.isSuccessful()... skipping it";
                }
              }
            }
          }
        });
      }
    }
    thread_pool.reset();
  }
};


///////////////////////////////////////////////////////////////
NetServer::NetServer(int port_,Type type_) : port(port_),type(type_)
{
  if (type==Http)
    pimpl=new HttpNetServer(port);
  else
    ThrowException("internal error");
}


///////////////////////////////////////////////////////////////
NetServer::~NetServer()
{
  if (pimpl) 
    delete pimpl;
}

} //namespace Visus