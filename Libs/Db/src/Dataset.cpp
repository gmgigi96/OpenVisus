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

#include <Visus/Dataset.h>
#include <Visus/DiskAccess.h>
#include <Visus/MultiplexAccess.h>
#include <Visus/CloudStorageAccess.h>
#include <Visus/RamAccess.h>
#include <Visus/FilterAccess.h>
#include <Visus/NetService.h>
#include <Visus/StringTree.h>
#include <Visus/Polygon.h>
#include <Visus/IdxFile.h>
#include <Visus/File.h>
#include <Visus/GoogleMapsDataset.h>
#include <Visus/GoogleMapsAccess.h>
#include <Visus/IdxHzOrder.h>
#include <Visus/IdxMultipleDataset.h>
#include <Visus/OnDemandAccess.h>
#include <Visus/ModVisusAccess.h>
#include <Visus/MandelbrotAccess.h>
#include <Visus/IdxMultipleAccess.h>
#include <Visus/IdxDiskAccess.h>
#include <Visus/IdxFilter.h>

namespace Visus {

VISUS_IMPLEMENT_SINGLETON_CLASS(DatasetFactory)

////////////////////////////////////////////////////////////////////
SharedPtr<Access> Dataset::createAccess(StringTree config,bool bForBlockQuery)
{
  if (!config.valid())
    config = getDefaultAccessConfig();

  String type =StringUtils::toLower(config.readString("type"));

  //I always need an access
  if (auto google = dynamic_cast<GoogleMapsDataset*>(this))
  {
    if (type.empty() || type == "GoogleMapsAccess")
    {
      SharedPtr<NetService> netservice;
      if (!bServerMode)
      {
        int nconnections = config.readInt("nconnections", 8);
        netservice = std::make_shared<NetService>(nconnections);
      }
      return std::make_shared<GoogleMapsAccess>(this, google->tiles_url, netservice);
    }
  }

  if (auto idx = dynamic_cast<IdxDataset*>(this))
  {
    //consider I can have thousands of childs (NOTE: this attribute should be "inherited" from child)
    auto midx = dynamic_cast<IdxMultipleDataset*>(this);
    if (midx)
      config.write("disable_async", true);

    String type = StringUtils::toLower(config.readString("type"));

    //no type, create default
    if (type.empty())
    {
      Url url = config.readString("url", getUrl());

      //local disk access
      if (url.isFile())
      {
        if (midx)
          return std::make_shared<IdxMultipleAccess>(midx, config);
        else
          return std::make_shared<IdxDiskAccess>(idx, config);
      }
      else
      {
        VisusAssert(url.isRemote());

        if (bForBlockQuery)
          return std::make_shared<ModVisusAccess>(this, config);
        else
          //I can execute box/point queries on the remote server
          return SharedPtr<Access>();
      }
    }

    //IdxDiskAccess
    if (type == "disk" || type == "idxdiskaccess")
      return std::make_shared<IdxDiskAccess>(idx, config);

    //IdxMultipleAccess
    if (type == "idxmultipleaccess" || type == "midx" || type == "multipleaccess")
    {
      VisusReleaseAssert(midx);
      return std::make_shared<IdxMultipleAccess>(midx, config);
    }

    //IdxMandelbrotAccess
    if (type == "idxmandelbrotaccess" || type == "mandelbrotaccess")
      return std::make_shared<MandelbrotAccess>(this, config);

    //ondemandaccess
    if (type == "ondemandaccess")
      return std::make_shared<OnDemandAccess>(this, config);
  }

  if (!config.valid()) {
    VisusAssert(!bForBlockQuery);
    return SharedPtr<Access>(); //pure remote query
  }

  if (type.empty()) {
    VisusAssert(false); //please handle this case in your dataset since I dont' know how to handle this situation here
    return SharedPtr<Access>();
  }

  //DiskAccess
  if (type=="diskaccess")
    return std::make_shared<DiskAccess>(this, config);

  // MULTIPLEX 
  if (type=="multiplex" || type=="multiplexaccess")
    return std::make_shared<MultiplexAccess>(this, config);
  
  // RAM CACHE 
  if (type == "lruam" || type == "ram" || type == "ramaccess")
  {
    auto ret = std::make_shared<RamAccess>(getDefaultBitsPerBlock());
    ret->can_read = StringUtils::contains(config.readString("chmod", Access::DefaultChMod), "r");
    ret->can_write = StringUtils::contains(config.readString("chmod", Access::DefaultChMod), "w");
    ret->setAvailableMemory(StringUtils::getByteSizeFromString(config.readString("available", "128mb")));
    return ret;
  }

  // NETWORK 
  if (type=="network" || type=="modvisusaccess")
    return std::make_shared<ModVisusAccess>(this, config);

  //CloudStorageAccess
  if (type=="cloudstorageaccess")
    return std::make_shared<CloudStorageAccess>(this, config);
  
  // FILTER 
  if (type=="filter" || type=="filteraccess")
    return std::make_shared<FilterAccess>(this, config);

  //problem here
  VisusAssert(false);
  return SharedPtr<Access>();
}

///////////////////////////////////////////////////////////
Field Dataset::getFieldEx(String fieldname) const
{
  //remove any params (they will be used in queries)
  ParseStringParams parse(fieldname);

  auto it=find_field.find(parse.without_params);
  if (it!=find_field.end())
  {
    Field ret=it->second;
    ret.name=fieldname; //important to keep the params! example "temperature?time=30"
    ret.params=parse.params;
    return ret;
  }

  //not found
  return Field();
}

///////////////////////////////////////////////////////////
Field Dataset::getField(String name) const {
  try {
    return getFieldEx(name);
  }
  catch (std::exception ex) {
    return Field();
  }
}

////////////////////////////////////////////////////////////////////////////////////
StringTree FindDatasetConfig(StringTree ar, String url)
{
  auto all_datasets = ar.getAllChilds("dataset");
  for (auto it : all_datasets)
  {
    if (it->readString("name") == url) {
      VisusAssert(it->hasAttribute("url"));
      return *it;
    }
  }

  for (auto it : all_datasets)
  {
    if (it->readString("url") == url)
      return *it;
  }

  auto ret = StringTree("dataset");
  ret.write("url", url);
  return ret;
}

/////////////////////////////////////////////////////////////////////////
SharedPtr<Dataset> LoadDatasetEx(StringTree ar)
{
  String url = ar.readString("url");
  if (!Url(url).valid())
    ThrowException("LoadDataset", url, "failed. Not a valid url");

  auto content = Utils::loadTextDocument(url);

  if (content.empty())
    ThrowException("empty content");

  //enrich ar by loaded document (ar has precedence)
  auto doc = StringTree::fromString(content);
  if (doc.valid())
  {
    //backward compatible
    if (doc.name == "midx")
    {
      doc.name = "dataset";
      doc.write("typename","IdxMultipleDataset");
    }

    StringTree::merge(ar, doc); //example <dataset tyname="IdxMultipleDataset">...</dataset>
    VisusReleaseAssert(ar.hasAttribute("typename"));
  }
  else
  {
    // backward compatible, old idx text format 
    ar.write("typename", "IdxDataset");

    IdxFile old_format;
    old_format.readFromOldFormat(content);
    ar.writeObject("idxfile", old_format);
  }

  auto TypeName = ar.getAttribute("typename");
  VisusReleaseAssert(!TypeName.empty());
  auto ret= DatasetFactory::getSingleton()->createInstance(TypeName);
  if (!ret)
    ThrowException("LoadDataset",url,"failed. Cannot DatasetFactory::getSingleton()->createInstance",TypeName);

  ret->readDatasetFromArchive(ar);
  return ret; 
}

////////////////////////////////////////////////
SharedPtr<Dataset> LoadDataset(String url) {
  auto ar = FindDatasetConfig(*DbModule::getModuleConfig(), url);
  return LoadDatasetEx(ar);
}


////////////////////////////////////////////////////////////////////////////////////
bool Dataset::insertSamples(LogicSamples Wsamples, Array Wbuffer, LogicSamples Rsamples, Array Rbuffer, Aborted aborted)
{
  if (!Wsamples.valid() || !Rsamples.valid())
    return false;

  if (Wbuffer.dtype != Rbuffer.dtype || Wbuffer.dims != Wsamples.nsamples || Rbuffer.dims != Rsamples.nsamples)
  {
    VisusAssert(false);
    return false;
  }

  //cannot find intersection (i.e. no sample to merge)
  BoxNi box = Wsamples.logic_box.getIntersection(Rsamples.logic_box);
  if (!box.isFullDim())
    return false;

  /*
  Example of the problem to solve:

  Wsamples:=-2 + kw*6         -2,          4,          10,            *16*,            22,            28,            34,            40,            *46*,            52,            58,  ...)
  Rsamples:=-4 + kr*5    -4,         1,        6,         11,         *16*,          21,       25,            ,31         36,          41,         *46*,          51,         56,       ...)

  give kl,kw,kr independent integers all >=0

  leastCommonMultiple(2,6,5)= 2*3*5 =30

  First "common" value (i.e. minimum value satisfying all 3 conditions) is 16
  */

  int pdim = Rbuffer.getPointDim();
  PointNi delta(pdim);
  for (int D = 0; D < pdim; D++)
  {
    Int64 lcm = Utils::leastCommonMultiple(Rsamples.delta[D], Wsamples.delta[D]);

    Int64 P1 = box.p1[D];
    Int64 P2 = box.p2[D];

    while (
      !Utils::isAligned(P1, Wsamples.logic_box.p1[D], Wsamples.delta[D]) ||
      !Utils::isAligned(P1, Rsamples.logic_box.p1[D], Rsamples.delta[D]))
    {
      //NOTE: if the value is already aligned, alignRight does nothing
      P1 = Utils::alignRight(P1, Wsamples.logic_box.p1[D], Wsamples.delta[D]);
      P1 = Utils::alignRight(P1, Rsamples.logic_box.p1[D], Rsamples.delta[D]);

      //cannot find any alignment, going beyond the valid range
      if (P1 >= P2)
        return false;

      //continue in the search IIF it's not useless
      if ((P1 - box.p1[D]) >= lcm)
      {
        //should be acceptable to be here, it just means that there are no samples to merge... 
        //but since 99% of the time Visus has pow-2 alignment it is high unlikely right now... adding the VisusAssert just for now
        VisusAssert(false);
        return false;
      }
    }

    delta[D] = lcm;
    P2 = Utils::alignRight(P2, P1, delta[D]);
    box.p1[D] = P1;
    box.p2[D] = P2;
  }

  VisusAssert(box.isFullDim());

  auto wfrom = Wsamples.logicToPixel(box.p1); auto wto = Wsamples.logicToPixel(box.p2); auto wstep = delta.rightShift(Wsamples.shift);
  auto rfrom = Rsamples.logicToPixel(box.p1); auto rto = Rsamples.logicToPixel(box.p2); auto rstep = delta.rightShift(Rsamples.shift);

  VisusAssert(PointNi::max(wfrom, PointNi(pdim)) == wfrom);
  VisusAssert(PointNi::max(rfrom, PointNi(pdim)) == rfrom);

  wto = PointNi::min(wto, Wbuffer.dims); wstep = PointNi::min(wstep, Wbuffer.dims);
  rto = PointNi::min(rto, Rbuffer.dims); rstep = PointNi::min(rstep, Rbuffer.dims);

  //first insert samples in the right position!
  if (!ArrayUtils::insert(Wbuffer, wfrom, wto, wstep, Rbuffer, rfrom, rto, rstep, aborted))
    return false;

  return true;
}



////////////////////////////////////////////////////////////////////
SharedPtr<BlockQuery> Dataset::createBlockQuery(BigInt blockid, Field field, double time, int mode, Aborted aborted)
{
  auto ret = std::make_shared<BlockQuery>();
  ret->dataset = this;
  ret->field = field;
  ret->time = time;
  ret->mode = mode; VisusAssert(mode == 'r' || mode == 'w');
  ret->aborted = aborted;
  ret->blockid = blockid;

  auto bitmask = getBitmask();
  auto bitsperblock = getDefaultBitsPerBlock();
  auto samplesperblock = 1 << bitsperblock;

  int H;
  if (bBlocksAreFullRes)
    H = blockid == 0 ? bitsperblock : bitsperblock + 0 + Utils::getLog2(1 + blockid);
  else
    H = blockid == 0 ? bitsperblock : bitsperblock + 1 + Utils::getLog2(0 + blockid);

  auto delta = block_samples[H].delta;

  //for first block I get twice the samples sice the blocking '1' can be '0' considering all previous levels from 'V'
  if (blockid==0 && !bBlocksAreFullRes)
    delta[bitmask[H]] >>= 1;

  PointNi p0;
  if (bBlocksAreFullRes)
  {
    Int64 first_block_in_level = (((Int64)1) << (H - bitsperblock)) - 1;
    auto coord = bitmask.deinterleave(blockid - first_block_in_level, H - bitsperblock);
    p0 = coord.innerMultiply(block_samples[H].logic_box.size());
  }
  else
  {
    p0 = HzOrder(bitmask).hzAddressToPoint(blockid * samplesperblock);
  }

  ret->H = H;
  ret->logic_samples = LogicSamples(block_samples[H].logic_box.translate(p0), delta);
  VisusAssert(ret->logic_samples.valid());
  return ret;
}

////////////////////////////////////////////////
void Dataset::executeBlockQuery(SharedPtr<Access> access,SharedPtr<BlockQuery> query)
{
  VisusAssert(access->isReading() || access->isWriting());

  int mode = query->mode; 
  auto failed = [&](String reason) {

    if (!access)
      query->setFailed();
    else
      mode == 'r'? access->readFailed(query) : access->writeFailed(query);
   
    PrintInfo("executeBlockQUery failed", reason);
    return;
  };

  if (!access)
    return failed("no access");

  if (!query->field.valid())
    return failed("field not valid");

  if (query->blockid < 0)
    return failed("address range not valid");

  if ((mode == 'r' && !access->can_read) || (mode == 'w' && !access->can_write))
    return failed("rw not enabled");

  if (!query->logic_samples.valid())
    return failed("logic_samples not valid");

  if (mode == 'w' && !query->buffer.valid())
    return failed("no buffer to write");

  // override time  from from field
  if (query->field.hasParam("time"))
    query->time = cdouble(query->field.getParam("time"));

  query->setRunning();

  if (mode == 'r')
  {
    access->readBlock(query);
    BlockQuery::readBlockEvent();
  }
  else
  {
    access->writeBlock(query);
    BlockQuery::writeBlockEvent();
  }

  return;
}





////////////////////////////////////////////////
SharedPtr<BoxQuery> Dataset::createBoxQuery(BoxNi logic_box, Field field, double time, int mode, Aborted aborted)
{
  auto ret = std::make_shared<BoxQuery>();
  ret->dataset = this;
  ret->field = field;
  ret->time = time;
  ret->mode = mode; VisusAssert(mode == 'r' || mode == 'w');
  ret->aborted = aborted;
  ret->logic_box = logic_box;
  ret->filter.domain = this->getLogicBox();
  return ret;
}

////////////////////////////////////////////////
std::vector<int> Dataset::guessBoxQueryEndResolutions(Frustum logic_to_screen, Position logic_position, int quality, int progression)
{
  if (!logic_position.valid())
    return {};

  auto maxh = getMaxResolution();
  auto endh = maxh;
  auto pdim = getPointDim();

  if (logic_to_screen.valid())
  {
    //important to work with orthogonal box
    auto logic_box = logic_position.toAxisAlignedBox();

    FrustumMap map(logic_to_screen);

    std::vector<Point2d> screen_points;
    for (auto p : logic_box.getPoints())
      screen_points.push_back(map.projectPoint(p.toPoint3()));

    //project on the screen
    std::vector<double> screen_distance = { 0,0,0 };

    for (auto edge : BoxNi::getEdges(pdim))
    {
      auto axis = edge.axis;
      auto s0 = screen_points[edge.index0];
      auto s1 = screen_points[edge.index1];
      auto Sd = s0.distance(s1);
      screen_distance[axis] = std::max(screen_distance[axis], Sd);
    }

    const int max_3d_texture_size = 2048;

    auto nsamples = logic_box.size().toPoint3();
    while (endh > 0)
    {
      std::vector<double> samples_per_pixel = {
        nsamples[0] / screen_distance[0],
        nsamples[1] / screen_distance[1],
        nsamples[2] / screen_distance[2]
      };

      std::sort(samples_per_pixel.begin(), samples_per_pixel.end());

      auto quality = sqrt(samples_per_pixel[0] * samples_per_pixel[1]);

      //note: in 2D samples_per_pixel[2] is INF; in 3D with an ortho view XY samples_per_pixel[2] is INF (see std::sort)
      bool bGood = quality < 1.0;

      if (pdim == 3 && bGood)
        bGood =
        nsamples[0] <= max_3d_texture_size &&
        nsamples[1] <= max_3d_texture_size &&
        nsamples[2] <= max_3d_texture_size;

      if (bGood)
        break;

      //by decreasing resolution I will get half of the samples on that axis
      auto bit = bitmask[endh];
      nsamples[bit] *= 0.5;
      --endh;
    }
  }

  //consider quality and progression
  endh = Utils::clamp(endh + quality, 0, maxh);

  std::vector<int> ret = { Utils::clamp(endh - progression, 0, maxh) };
  while (ret.back() < endh)
    ret.push_back(Utils::clamp(ret.back() + pdim, 0, endh));

  if (auto google = dynamic_cast<GoogleMapsDataset*>(this))
  {
    for (auto& it : ret)
      it = (it >> 1) << 1; //TODO: google maps does not have odd resolutions
  }

  return ret;
}

//////////////////////////////////////////////////////////////
void Dataset::beginBoxQuery(SharedPtr<BoxQuery> query)
{
  if (!query)
    return;

  if (query->getStatus() != Query::QueryCreated)
    return;

  if (query->aborted())
    return query->setFailed("query aborted");

  if (!query->field.valid())
    return query->setFailed("field not valid");

  // override time from field
  if (query->field.hasParam("time"))
    query->time = cdouble(query->field.getParam("time"));

  if (!getTimesteps().containsTimestep(query->time))
    return query->setFailed("wrong time");

  if (!query->logic_box.valid())
    return query->setFailed("query logic_box not valid");

  if (!query->logic_box.getIntersection(this->getLogicBox()).isFullDim())
    return query->setFailed("user_box not valid");

  if (query->end_resolutions.empty())
    query->end_resolutions = { this->getMaxResolution() };

  //google has only even resolution
  if (auto google = dynamic_cast<GoogleMapsDataset*>(this))
  {
    std::set<int> good;
    for (auto it : query->end_resolutions)
    {
      auto value = (it >> 1) << 1;
      good.insert(Utils::clamp(value, getDefaultBitsPerBlock(), getMaxResolution()));
    }

    query->end_resolutions = std::vector<int>(good.begin(), good.end());
  }

  //end resolution
  for (auto it : query->end_resolutions)
  {
    if (it <0 || it> this->getMaxResolution())
      return query->setFailed("wrong end resolution");
  }

  //start_resolution
  if (query->start_resolution > 0)
  {
    if (query->end_resolutions.size() != 1 || query->start_resolution != query->end_resolutions[0])
      return query->setFailed("wrong query start resolution");
  }

  //filters?
  if (query->filter.enabled)
  {
    if (!query->filter.dataset_filter)
    {
      query->filter.dataset_filter = createFilter(query->field);

      if (!query->filter.dataset_filter)
        query->disableFilters();
    }
  }

  for (auto it : query->end_resolutions)
  {
    if (setBoxQueryEndResolution(query, it))
      return query->setRunning();
  }

  query->setFailed();
}

//////////////////////////////////////////////////////////////
bool Dataset::executeBoxQuery(SharedPtr<Access> access, SharedPtr<BoxQuery> query)
{
  VisusReleaseAssert(bBlocksAreFullRes);

  if (!query)
    return false;

  if (!(query->isRunning() && query->getCurrentResolution() < query->getEndResolution()))
    return false;

  if (query->aborted())
  {
    query->setFailed("query aborted");
    return false;
  }

  //for 'r' queries I can postpone the allocation
  if (query->mode == 'w')
  {
    query->setFailed("write not supported");
    return false;
  }

  if (!query->allocateBufferIfNeeded())
  {
    query->setFailed("cannot allocate buffer");
    return false;
  }

  //always need an access.. the google server cannot handle pure remote queries (i.e. compose the tiles on server side)
  if (!access)
    access = createAccessForBlockQuery();

  int end_resolution = query->end_resolution;

  WaitAsync< Future<Void> > wait_async;

  BoxNi box = this->getLogicBox();

  std::stack< std::tuple<BoxNi, BigInt, int> > stack;
  stack.push({ box ,0,this->getDefaultBitsPerBlock() });

  access->beginRead();
  while (!stack.empty() && !query->aborted())
  {
    auto top = stack.top();
    stack.pop();

    auto box = std::get<0>(top);
    auto blockid = std::get<1>(top);
    auto H = std::get<2>(top);

    if (!box.getIntersection(query->logic_box).isFullDim())
      continue;

    //is the resolution I need?
    if (H == end_resolution)
    {
      auto block_query = createBlockQuery(blockid, query->field, query->time, 'r', query->aborted);

      executeBlockQuery(access, block_query);
      wait_async.pushRunning(block_query->done).when_ready([this, query, block_query](Void)
      {
        if (query->aborted() || !block_query->ok())
          return;

        mergeBoxQueryWithBlockQuery(query, block_query);
      });
    }
    else
    {
      int bitsperblock = this->getDefaultBitsPerBlock();
      auto split_bit = bitmask[1 + H - bitsperblock];
      auto middle = (box.p1[split_bit] + box.p2[split_bit]) >> 1;
      auto lbox = box; lbox.p2[split_bit] = middle;
      auto rbox = box; rbox.p1[split_bit] = middle;
      stack.push(std::make_tuple(rbox, blockid * 2 + 2, H + 1));
      stack.push(std::make_tuple(lbox, blockid * 2 + 1, H + 1));
    }
  }
  access->endRead();

  wait_async.waitAllDone();

  if (query->aborted())
  {
    query->setFailed("query aborted");
    return false;
  }

  query->setCurrentResolution(query->end_resolution);
  return true;
}

//////////////////////////////////////////////////////////////
bool Dataset::mergeBoxQueryWithBlockQuery(SharedPtr<BoxQuery> query, SharedPtr<BlockQuery> blockquery)
{
  VisusAssert(blockquery->buffer.layout.empty());
  return insertSamples(query->logic_samples, query->buffer, blockquery->logic_samples, blockquery->buffer, query->aborted);
}

//////////////////////////////////////////////////////////////
void Dataset::nextBoxQuery(SharedPtr<BoxQuery> query)
{
  VisusReleaseAssert(bBlocksAreFullRes);

  if (!query)
    return;

  if (!(query->isRunning() && query->getCurrentResolution() == query->getEndResolution()))
    return;

  //reached the end?
  if (query->end_resolution == query->end_resolutions.back())
    return query->setOk();

  int index = Utils::find(query->end_resolutions, query->end_resolution);

  if (!setBoxQueryEndResolution(query, query->end_resolutions[index + 1]))
    VisusReleaseAssert(false); //should not happen

  //merging is not supported, so I'm resetting the buffer
  query->buffer = Array();
}

//////////////////////////////////////////////////////////////
bool Dataset::setBoxQueryEndResolution(SharedPtr<BoxQuery> query, int value)
{
  VisusReleaseAssert(bBlocksAreFullRes);

  VisusAssert(query->end_resolution < value);
  query->end_resolution = value;

  auto end_resolution = query->end_resolution;
  auto user_box = query->logic_box.getIntersection(this->getLogicBox());
  VisusAssert(user_box.isFullDim());

  auto Lsamples = level_samples[end_resolution];
  auto box = Lsamples.alignBox(user_box);

  if (!box.isFullDim())
    return false;

  query->logic_samples = LogicSamples(box, Lsamples.delta);
  return true;

}

/////////////////////////////////////////////////////////////////////////
NetRequest Dataset::createBoxQueryRequest(SharedPtr<BoxQuery> query)
{
  /*
    *****NOTE FOR REMOTE QUERIES:*****

    I always restart from scratch so I will do Query0[0,resolutions[0]], Query1[0,resolutions[1]], Query2[0,resolutions[2]]  without any merging
    In this way I transfer a little more data on the network (without compression in the worst case the ratio is 2.0)
    but I can use lossy compression and jump levels
    in the old code I was using:

      Query0[0,resolutions[0]  ]
      Query1[0,resolutions[0]+1] <-- by merging prev_single and Query[resolutions[0]+1,resolutions[0]+1]
      Query1[0,resolutions[0]+2] <-- by merging prev_single and Query[resolutions[0]+2,resolutions[0]+2]
      ...

        -----------------------------
        | overall     |single       |
        | --------------------------|
    Q0  | 2^0*T       |             |
    Q1  | 2^1*T       | 2^0*T       |
    ..  |             |             |
    Qn  | 2^n*T       | 2^(n-1)*T   |
        -----------------------------

    OLD CODE transfers singles = T*(2^0+2^1+...+2^(n-1))=T*2^n    (see http://it.wikipedia.org/wiki/Serie_geometrica)
    NEW CODE transfers overall = T*(2^0+2^1+...+2^(n  ))=T*2^n+1

    RATIO:=overall_transfer/single_transfer=2.0

    With the new code I have the following advantages:

        (*) I don't have to go level by level after Q0. By "jumping" levels I send less data
        (*) I can use lossy compression (in the old code I needed lossless compression to rebuild the data for Qi with i>0)
        (*) not all datasets support the merging (see IdxMultipleDataset and GoogleMapsDataset)
        (*) the filters (example wavelets) are applied always on the server side (in the old code filters were applied on the server only for Q0)
  */


  VisusAssert(query->mode == 'r');

  Url url = this->getUrl();

  NetRequest ret;
  ret.url = url.getProtocol() + "://" + url.getHostname() + ":" + cstring(url.getPort()) + "/mod_visus";
  ret.url.params = url.params;  //I may have some extra params I want to keep!
  ret.url.setParam("action", "boxquery");
  ret.url.setParam("dataset", url.getParam("dataset"));
  ret.url.setParam("time", url.getParam("time", cstring(query->time)));
  ret.url.setParam("compression", url.getParam("compression", "zip")); //for networking I prefer to use zip
  ret.url.setParam("field", query->field.name);
  ret.url.setParam("fromh", cstring(query->start_resolution));
  ret.url.setParam("toh", cstring(query->getEndResolution()));
  ret.url.setParam("maxh", cstring(getMaxResolution())); //backward compatible
  ret.url.setParam("box", query->logic_box.toOldFormatString());
  ret.aborted = query->aborted;
  return ret;
}

////////////////////////////////////////////////////////////////////
bool Dataset::executeBoxQueryOnServer(SharedPtr<BoxQuery> query)
{
  auto request = createBoxQueryRequest(query);
  if (!request.valid())
  {
    query->setFailed("cannot create box query request");
    return false;
  }

  auto response = NetService::getNetResponse(request);
  if (!response.isSuccessful())
  {
    query->setFailed(cstring("network request failed", cnamed("errormsg", response.getErrorMessage())));
    return false;
  }

  auto decoded = response.getCompatibleArrayBody(query->getNumberOfSamples(), query->field.dtype);
  if (!decoded.valid()) {
    query->setFailed("failed to decode body");
    return false;
  }

  query->buffer = decoded;
  query->setCurrentResolution(query->end_resolution);
  return true;
}



/////////////////////////////////////////////////////////
SharedPtr<PointQuery> Dataset::createPointQuery(Position logic_position, Field field, double time, Aborted aborted)
{
  VisusAssert(!bBlocksAreFullRes);
  auto ret = std::make_shared<PointQuery>();
  ret->dataset = this;
  ret->field = field = field;
  ret->time = time;
  ret->mode = 'r';
  ret->aborted = aborted;
  ret->logic_position = logic_position;
  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////
void Dataset::beginPointQuery(SharedPtr<PointQuery> query)
{
  VisusAssert(!bBlocksAreFullRes);

  if (!query)
    return;

  if (query->getStatus() != Query::QueryCreated)
    return;

  //if you want to set a buffer for 'w' queries, please do it after begin
  VisusAssert(!query->buffer.valid());

  if (getPointDim() != 3)
    return query->setFailed("pointquery supported only in 3d so far");

  if (!query->field.valid())
    return query->setFailed("field not valid");

  if (!query->logic_position.valid())
    return query->setFailed("position not valid");

  // override time from field
  if (query->field.hasParam("time"))
    query->time = cdouble(query->field.getParam("time"));

  if (!getTimesteps().containsTimestep(query->time))
    return query->setFailed("wrong time");

  query->end_resolution = query->end_resolutions.front();
  query->setRunning();
}

//////////////////////////////////////////////////////////////////////////////////////////
void Dataset::nextPointQuery(SharedPtr<PointQuery> query)
{
  VisusAssert(!bBlocksAreFullRes);

  if (!query)
    return;

  if (!(query->isRunning() && query->getCurrentResolution() == query->getEndResolution()))
    return;

  //reached the end? 
  if (query->end_resolution == query->end_resolutions.back())
    return query->setOk();

  int index = Utils::find(query->end_resolutions, query->end_resolution);
  query->end_resolution = query->end_resolutions[index + 1];
}

/// ///////////////////////////////////////////////////////////////////////////
std::vector<int> Dataset::guessPointQueryEndResolutions(Frustum logic_to_screen, Position logic_position, int quality, int progression)
{
  VisusAssert(!bBlocksAreFullRes);

  if (!logic_position.valid())
    return {};

  auto maxh = getMaxResolution();
  auto endh = maxh;
  auto pdim = getPointDim();

  if (logic_to_screen.valid())
  {
    std::vector<Point3d> logic_points;
    std::vector<Point2d> screen_points;
    FrustumMap map(logic_to_screen);
    for (auto p : logic_position.getPoints())
    {
      auto logic_point = p.toPoint3();
      logic_points.push_back(logic_point);
      screen_points.push_back(map.projectPoint(logic_point));
    }

    // valerio's algorithm, find the final view dependent resolution (endh)
    // (the default endh is the maximum resolution available)
    BoxNi::Edge longest_edge;
    double longest_screen_distance = NumericLimits<double>::lowest();
    for (auto edge : BoxNi::getEdges(pdim))
    {
      double screen_distance = (screen_points[edge.index1] - screen_points[edge.index0]).module();

      if (screen_distance > longest_screen_distance)
      {
        longest_edge = edge;
        longest_screen_distance = screen_distance;
      }
    }

    //I match the highest resolution on dataset axis (it's just an euristic!)
    for (int A = 0; A < pdim; A++)
    {
      double logic_distance = fabs(logic_points[longest_edge.index0][A] - logic_points[longest_edge.index1][A]);
      double samples_per_pixel = logic_distance / longest_screen_distance;
      Int64  num = Utils::getPowerOf2((Int64)samples_per_pixel);
      while (num > samples_per_pixel)
        num >>= 1;

      int H = maxh;
      for (; num > 1 && H >= 0; H--)
      {
        if (bitmask[H] == A)
          num >>= 1;
      }

      endh = std::min(endh, H);
    }
  }

  //consider quality and progression
  endh = Utils::clamp(endh + quality, 0, maxh);

  std::vector<int> ret = { Utils::clamp(endh - progression, 0, maxh) };
  while (ret.back() < endh)
    ret.push_back(Utils::clamp(ret.back() + pdim, 0, endh));

  return ret;
}

/// ///////////////////////////////////////////////////////////////////////////
PointNi Dataset::guessPointQueryNumberOfSamples(Frustum logic_to_screen, Position logic_position, int end_resolution)
{
  VisusAssert(!bBlocksAreFullRes);

  //*********************************************************************
  // valerio's algorithm, find the final view dependent resolution (endh)
  // (the default endh is the maximum resolution available)
  //*********************************************************************

  auto bitmask = this->idxfile.bitmask;
  int pdim = bitmask.getPointDim();

  if (!logic_position.valid())
    return PointNi(pdim);

  const int unit_box_edges[12][2] =
  {
    {0,1}, {1,2}, {2,3}, {3,0},
    {4,5}, {5,6}, {6,7}, {7,4},
    {0,4}, {1,5}, {2,6}, {3,7}
  };

  std::vector<Point3d> logic_points;
  for (auto p : logic_position.getPoints())
    logic_points.push_back(p.toPoint3());

  std::vector<Point2d> screen_points;
  if (logic_to_screen.valid())
  {
    FrustumMap map(logic_to_screen);
    for (int I = 0; I < 8; I++)
      screen_points.push_back(map.projectPoint(logic_points[I]));
  }

  PointNi virtual_worlddim = PointNi::one(pdim);
  for (int H = 1; H <= end_resolution; H++)
  {
    int bit = bitmask[H];
    virtual_worlddim[bit] <<= 1;
  }

  PointNi nsamples = PointNi::one(pdim);
  for (int E = 0; E < 12; E++)
  {
    int query_axis = (E >= 8) ? 2 : (E & 1 ? 1 : 0);
    Point3d P1 = logic_points[unit_box_edges[E][0]];
    Point3d P2 = logic_points[unit_box_edges[E][1]];
    Point3d edge_size = (P2 - P1).abs();

    PointNi idx_size = this->getLogicBox().size();

    // need to project onto IJK  axis
    // I'm using this formula: x/virtual_worlddim[dataset_axis] = factor = edge_size[dataset_axis]/idx_size[dataset_axis]
    for (int dataset_axis = 0; dataset_axis < 3; dataset_axis++)
    {
      double factor = (double)edge_size[dataset_axis] / (double)idx_size[dataset_axis];
      Int64 x = (Int64)(virtual_worlddim[dataset_axis] * factor);
      nsamples[query_axis] = std::max(nsamples[query_axis], x);
    }
  }

  //view dependent, limit the nsamples to what the user can see on the screen!
  if (!screen_points.empty())
  {
    PointNi view_dependent_dims = PointNi::one(pdim);
    for (int E = 0; E < 12; E++)
    {
      int query_axis = (E >= 8) ? 2 : (E & 1 ? 1 : 0);
      Point2d p1 = screen_points[unit_box_edges[E][0]];
      Point2d p2 = screen_points[unit_box_edges[E][1]];
      double pixel_distance_on_screen = (p2 - p1).module();
      view_dependent_dims[query_axis] = std::max(view_dependent_dims[query_axis], (Int64)pixel_distance_on_screen);
    }

    nsamples[0] = std::min(view_dependent_dims[0], nsamples[0]);
    nsamples[1] = std::min(view_dependent_dims[1], nsamples[1]);
    nsamples[2] = std::min(view_dependent_dims[2], nsamples[2]);
  }

  //important
  nsamples = nsamples.compactDims();
  nsamples.setPointDim(3, 1);

  return nsamples;
}

//////////////////////////////////////////////////////////////////////////////////////////
NetRequest Dataset::createPointQueryRequest(SharedPtr<PointQuery> query)
{
  Url url = this->getUrl();

  NetRequest request;
  request.url = url.getProtocol() + "://" + url.getHostname() + ":" + cstring(url.getPort()) + "/mod_visus";
  request.url.params = url.params;  //I may have some extra params I want to keep!
  request.url.setParam("action", "pointquery");
  request.url.setParam("dataset", url.getParam("dataset"));
  request.url.setParam("time", url.getParam("time", cstring(query->time)));
  request.url.setParam("compression", url.getParam("compression", "zip")); //for networking I prefer to use zip
  request.url.setParam("field", query->field.name);
  request.url.setParam("fromh", cstring(0)); //backward compatible
  request.url.setParam("toh", cstring(query->end_resolution));
  request.url.setParam("maxh", cstring(getMaxResolution())); //backward compatible
  request.url.setParam("matrix", query->logic_position.getTransformation().toString());
  request.url.setParam("box", query->logic_position.getBoxNd().toBox3().toString(/*bInterleave*/false));
  request.url.setParam("nsamples", query->getNumberOfPoints().toString());
  request.aborted = query->aborted;

  return request;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool Dataset::executePointQueryOnServer(SharedPtr<PointQuery> query)
{
  auto request = createPointQueryRequest(query);
  if (!request.valid())
  {
    query->setFailed("cannot create point query request");
    return false;
  }

  PrintInfo(request.url);
  auto response = NetService::getNetResponse(request);
  if (!response.isSuccessful())
  {
    query->setFailed(cstring("network request failed ", cnamed("errormsg", response.getErrorMessage())));
    return false;
  }

  auto decoded = response.getCompatibleArrayBody(query->getNumberOfPoints(), query->field.dtype);
  if (!decoded.valid()) {
    query->setFailed("failed to decode body");
    return false;
  }

  query->buffer = decoded;

  if (query->aborted()) {
    query->setFailed("query aborted");
    return false;
  }

  query->cur_resolution = query->end_resolution;
  return true;
}




///////////////////////////////////////////////////////////////////////////////////
BoxNi Dataset::adjustBoxQueryFilterBox(BoxQuery* query, IdxFilter* filter, BoxNi user_box, int H)
{
  VisusAssert(!bBlocksAreFullRes);

  //there are some case when I need alignment with pow2 box, for example when doing kdquery=box with filters
  auto bitmask = idxfile.bitmask;
  int pdim = bitmask.getPointDim();

  PointNi delta = this->level_samples[H].delta;

  BoxNi domain = query->filter.domain;

  //important! for the filter alignment
  BoxNi box = user_box.getIntersection(domain);

  if (!box.isFullDim())
    return box;

  PointNi filterstep = filter->getFilterStep(H);

  for (int D = 0; D < pdim; D++)
  {
    //what is the world step of the filter at the current resolution
    Int64 FILTERSTEP = filterstep[D];

    //means only one sample so no alignment
    if (FILTERSTEP == 1)
      continue;

    box.p1[D] = Utils::alignLeft(box.p1[D], (Int64)0, FILTERSTEP);
    box.p2[D] = Utils::alignLeft(box.p2[D] - 1, (Int64)0, FILTERSTEP) + FILTERSTEP;
  }

  //since I've modified the box I need to do the intersection with the box again
  //important: this intersection can cause a misalignment, but applyToQuery will handle it (see comments)
  box = box.getIntersection(domain);
  return box;
}

///////////////////////////////////////////////////////////////////////////////
bool Dataset::computeFilter(SharedPtr<IdxFilter> filter, double time, Field field, SharedPtr<Access> access, PointNi SlidingWindow, bool bVerbose)
{
  VisusAssert(!bBlocksAreFullRes);

  //this works only for filter_size==2, otherwise the building of the sliding_window is very difficult
  VisusAssert(filter->size == 2);

  DatasetBitmask bitmask = this->idxfile.bitmask;
  BoxNi          box = this->getLogicBox();

  int pdim = bitmask.getPointDim();

  //the window size must be multiple of 2, otherwise I loose the filter alignment
  for (int D = 0; D < pdim; D++)
  {
    Int64 size = SlidingWindow[D];
    VisusAssert(size == 1 || (size / 2) * 2 == size);
  }

  //convert the dataset  FINE TO COARSE, this can be really time consuming!!!!
  for (int H = this->getMaxResolution(); H >= 1; H--)
  {
    if (bVerbose)
      PrintInfo("Applying filter to dataset resolution", H);

    int bit = bitmask[H];

    Int64 FILTERSTEP = filter->getFilterStep(H)[bit];

    //need to align the from so that the first sample is filter-aligned
    PointNi From = box.p1;

    if (!Utils::isAligned(From[bit], (Int64)0, FILTERSTEP))
      From[bit] = Utils::alignLeft(From[bit], (Int64)0, FILTERSTEP) + FILTERSTEP;

    PointNi To = box.p2;
    for (auto P = ForEachPoint(From, To, SlidingWindow); !P.end(); P.next())
    {
      //this is the sliding window
      BoxNi sliding_window(P.pos, P.pos + SlidingWindow);

      //important! crop to the stored world box to be sure that the alignment with the filter is correct!
      sliding_window = sliding_window.getIntersection(box);

      //no valid box since do not intersect with box 
      if (!sliding_window.isFullDim())
        continue;

      //I'm sure that since the From is filter-aligned, then P must be already aligned
      VisusAssert(Utils::isAligned(sliding_window.p1[bit], (Int64)0, FILTERSTEP));

      //important, i'm not using adjustBox because I'm sure it is already correct!
      auto read = createBoxQuery(sliding_window, field, time, 'r');
      read->setResolutionRange(0, H);

      beginBoxQuery(read);

      if (!executeBoxQuery(access, read))
        return false;

      //if you want to debug step by step...
#if 0
      {
        PrintInfo("Before");
        int nx = (int)read->buffer->dims.x;
        int ny = (int)read->buffer->dims.y;
        Uint8* SRC = read->buffer->c_ptr();
        std::ostringstream out;
        for (int Y = 0; Y < ny; Y++)
        {
          for (int X = 0; X < nx; X++)
          {
            out << std::setw(3) << (int)SRC[((ny - Y - 1) * nx + X) * 2] << " ";
          }
          out << std::endl;
        }
        out << std::endl;
        PrintInfo("\n" << out.str();
      }
#endif

      filter->internalComputeFilter(read.get(),/*bInverse*/false);

      //if you want to debug step by step...
#if 0 
      {
        PrintInfo("After");
        int nx = (int)read->buffer->dims.x;
        int ny = (int)read->buffer->dims.y;
        Uint8* SRC = read->buffer->c_ptr();
        std::ostringstream out;
        for (int Y = 0; Y < ny; Y++)
        {
          for (int X = 0; X < nx; X++)
          {
            out << std::setw(3) << (int)SRC[((ny - Y - 1) * nx + X) * 2] << " ";
          }
          out << std::endl;
        }
        out << std::endl;
        PrintInfo("\n", out.str());
      }
#endif

      auto write = createBoxQuery(sliding_window, field, time, 'w');
      write->setResolutionRange(0, H);

      beginBoxQuery(write);

      if (!write->isRunning())
        return false;

      write->buffer = read->buffer;

      if (!executeBoxQuery(access, write))
        return false;
    }

    //I'm going to write the next resolution, double the dimension along the current axis
    //in this way I have the same number of samples!
    SlidingWindow[bit] <<= 1;
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////
void Dataset::computeFilter(const Field& field, int window_size, bool bVerbose)
{
  VisusAssert(!bBlocksAreFullRes);

  if (bVerbose)
    PrintInfo("starting filter computation...");

  auto filter = createFilter(field);

  //the filter will be applied using this sliding window
  PointNi sliding_box = PointNi::one(getPointDim());
  for (int D = 0; D < getPointDim(); D++)
    sliding_box[D] = window_size;

  auto acess = createAccess();
  for (auto time : getTimesteps().asVector())
    computeFilter(filter, time, field, acess, sliding_box, bVerbose);
}




/// ////////////////////////////////////////////////////////////////
void Dataset::readDatasetFromArchive(Archive& ar)
{
  IdxFile idxfile;
  ar.readObject("idxfile", idxfile);

  String url = ar.readString("url");
  idxfile.validate(url);

  this->dataset_body = ar;
  this->kdquery_mode = KdQueryMode::fromString(ar.readString("kdquery", Url(url).getParam("kdquery")));
  this->idxfile = idxfile;
  this->bitmask = idxfile.bitmask;
  this->default_bitsperblock = idxfile.bitsperblock;
  this->logic_box = idxfile.logic_box;
  this->timesteps = idxfile.timesteps;
  setDatasetBounds(idxfile.bounds);

  //idxfile.fields -> Dataset::fields 
  if (this->fields.empty())
  {
    for (auto field : idxfile.fields)
    {
      if (field.name != "__fake__")
        addField(field);
    }
  }

  //create samples for levels and blocks
  {
    //bitmask = DatasetBitmask::fromString("V0011");
    //bitsperblock = 2;

    int bitsperblock = getDefaultBitsPerBlock();
    int pdim = bitmask.getPointDim();
    auto MaxH = bitmask.getMaxResolution();

    level_samples.clear(); level_samples.push_back(LogicSamples(bitmask.getPow2Box(), bitmask.getPow2Dims()));
    block_samples.clear(); block_samples.push_back(LogicSamples(bitmask.getPow2Box(), bitmask.getPow2Dims()));

    for (int H = 1; H <= MaxH; H++)
    {
      BoxNi logic_box;
      auto delta = PointNi::one(pdim);
      auto block_nsamples = PointNi::one(pdim);
      auto level_nsamples = PointNi::one(pdim);

      if (this->bBlocksAreFullRes)
      {
        //goint right to left (0,H] counting the bits
        for (int K = H; K > 0; K--)
          level_nsamples[bitmask[K]] *= 2;

        //delta.go from right to left up to the free '0' excluded
        for (int K = MaxH; K > H; K--)
          delta[bitmask[K]] *= 2;

        //block_nsamples. go from free '0' included to left up to bitsperblock
				for (int K = H, I = 0; I < bitsperblock && K>0; I++, K--)
					block_nsamples[bitmask[K]] *= 2;

        //logic_box
        logic_box = bitmask.getPow2Box();
      }
      else
      {
        HzOrder hzorder(bitmask);

        //goint right to left (0,H-1] counting the bits
        for (int K = H-1; K > 0; K--)
          level_nsamples[bitmask[K]] *= 2;

        //compute delta. go from right to left up to the blocking '1' included
        for (int K = MaxH; K >= H; K--)
          delta[bitmask[K]] *= 2;

        //block_nsamples. go from blocking '1' excluded to left up to bitsperblock
        for (int K = H-1, I = 0; I < bitsperblock && K>0; I++, K--)
          block_nsamples[bitmask[K]] *= 2;

				logic_box = BoxNi(hzorder.getLevelP1(H), hzorder.getLevelP2Included(H) + delta);
      }

      level_samples.push_back(LogicSamples(logic_box, delta)); 
      block_samples.push_back(LogicSamples(BoxNi(PointNi::zero(pdim), block_nsamples.innerMultiply(delta)), delta));

      VisusReleaseAssert(level_samples.back().nsamples==level_nsamples);


#if 0
      if (H >= bitsperblock)
      {
        PrintInfo("H", H,
          "level_samples[H].delta", level_samples[H].delta,
          "block_samples[H].nsamples", block_samples[H].nsamples);
      }
#endif
    }
  }
}

} //namespace Visus 
