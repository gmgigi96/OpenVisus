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

#include <Visus/Thread.h>


#define ENABLE_CONCURRENCY_VISUALIZER 0

#if ENABLE_CONCURRENCY_VISUALIZER
#include <C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\Extensions\xwsx55s0.r1e\SDK\Native\Inc\cvmarkers.h>
#endif

namespace Visus {

//////////////////////////////////////////////////////////////
#if ENABLE_CONCURRENCY_VISUALIZER
class ConcurrencyVisualizer::Pimpl
{
public:

  class Series
  {
  public:

    PCV_MARKERSERIES series;
    PCV_PROVIDER     provider;

    //constructor
    Series()
    {
      CvInitProvider(&CvDefaultProviderGuid, &provider);
      CvCreateMarkerSeries(provider, "", &series);
    }

    //destructor
    ~Series() {

      CvReleaseMarkerSeries(series);
      CvReleaseProvider(provider);
    }

    //getSingleton
    static PCV_MARKERSERIES& getSingleton() {
      static Series ret;
      return ret.series;
    }

  };

  String   name;
  PCV_SPAN span;

  //constructor
  Pimpl(String name_): name(name_){
    CvEnterSpan(Series::getSingleton(), &span, name.c_str());
  }

  //destructor
  ~Pimpl() {
    CvLeaveSpan(span);
  }
};
#endif

//////////////////////////////////////////////////////////////
ConcurrencyVisualizer::ConcurrencyVisualizer(String name)
{
#if ENABLE_CONCURRENCY_VISUALIZER
  pimpl = new Pimpl(name);
#endif
}

//destructor
ConcurrencyVisualizer::~ConcurrencyVisualizer()\
{
#if ENABLE_CONCURRENCY_VISUALIZER
  delete pimpl;
#endif
}

////////////////////////////////////////////////////////////
SharedPtr<std::thread> Thread::start(String thread_name,std::function<void()> entry_proc)
{
  ++ApplicationStats::num_threads;
  
  return std::make_shared<std::thread>(([entry_proc, thread_name]()
  {
    ConcurrencyVisualizer cv(thread_name);
    entry_proc(); 
    --ApplicationStats::num_threads;
  }));
}

} //namespace Visus