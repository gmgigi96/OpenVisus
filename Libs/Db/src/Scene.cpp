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

#include <Visus/Scene.h>
#include <Visus/DiskAccess.h>
#include <Visus/NetService.h>
#include <Visus/VisusConfig.h>


namespace Visus {

 ////////////////////////////////////////////////////////////////////////////////////
Scene::Info Scene::findSceneInVisusConfig(String name)
{
  Scene::Info ret;
  ret.name=name;

  auto all_scenes=VisusConfig::findAllChildsWithName("scene");

  bool bFound=false;
  for (auto it : all_scenes)
  {
    if (it->readString("name")==name) 
    {
      ret.url=it->readString("url");
      ret.config=*it;
      bFound=true;
      break;
    }
  }

  if (!bFound)
  {
    for (auto it : all_scenes)
    {
      if (it->readString("url")==name) 
      {
        ret.url=it->readString("url");
        ret.config=*it;
        bFound=true;
        break;
      }
    }
  }

  if (!bFound)
    ret.url=Url(name);

  if (!ret.url.valid())
  {
    VisusWarning() << "Scene::loadScene(" << name << ") failed. Not a valid url";
    return Info();
  }

  //local
  if (ret.url.isFile())
  {
    String extension=Path(ret.url.getPath()).getExtension();
//    ret.TypeName=DatasetPluginFactory::getSingleton()->getRegisteredDatasetType(extension);
//
//    //probably not even an idx dataset
//    if (ret.TypeName.empty()) 
//      return Info(); 
  }
  else if (StringUtils::contains(ret.url.toString(),"mod_visus"))
  {
    ret.url.setParam("action","readscene");
      
    auto response=NetService::getNetResponse(ret.url);
    if (!response.isSuccessful())
    {
      VisusWarning()<<"LoadScene("<<ret.url.toString()<<") failed errormsg("<<response.getErrorMessage()<<")";
      return Info();
    }
    
    VisusWarning() << response.getTextBody();
    VisusWarning() << response.toString();
      
//    ret.TypeName = response.getHeader("visus-typename","IdxDataset");
//    if (ret.TypeName.empty())
//    {
//      VisusWarning()<<"LoadDataset("<<ret.url.toString()<<") failed. Got empty TypeName";
//      return Info();
//    }
//
//    // backward compatible 
//    if (ret.TypeName == "MultipleDataset")
//      ret.TypeName="IdxMultipleDataset";
  }
  
  return ret;
}

//////////////////////////////////////////////////////////////////////////////
bool Scene::openFromUrl(Url url)
{
  String content;
//  {
//    //special case for cloud storage, I need to sign the request
//    if (url.isRemote() && !StringUtils::contains(url.toString(),"mod_visus"))
//    {
//      UniquePtr<CloudStorage> cloud_storage(CloudStorage::createInstance(url)); VisusAssert(cloud_storage);
//      if (cloud_storage)
//        content=NetService::getNetResponse(cloud_storage->createGetBlobRequest(url)).getTextBody();
//    }
//    else
//    {
      content=Utils::loadTextDocument(url.toString());
//    }
//  }
  
  if (content.empty()){
    VisusError()<<"scene file empty";
    return false;
  }
  
  StringTree stree;
  if (!stree.loadFromXml(content))
  {
    VisusError()<<"scene file is wrong";
    VisusAssert(false);
    return false;
  }
  
  scene_body = stree.toString();
  
  return true;
}

/////////////////////////////////////////////////////////////////////////
SharedPtr<Scene> Scene::loadScene(String name)
{
  if (name.empty())
    return SharedPtr<Scene>();

  auto info=findSceneInVisusConfig(name);
  if (!info.valid())
    return SharedPtr<Scene>();

//  String TypeName = info.TypeName;
  Url    url      = info.url;

//  if (TypeName.empty())
//    return SharedPtr<Dataset>();

  SharedPtr<Scene> ret = SharedPtr<Scene>(new Scene());
  //(ObjectFactory::getSingleton()->createInstance<Scene>(TypeName));
//  if (!ret) 
//  {
//    VisusWarning()<<"Dataset::loadDataset("<<url.toString()<<") failed. Cannot ObjectFactory::getSingleton()->createInstance("<<TypeName<<")";
//    return SharedPtr<Dataset>();
//  }

//  ret->config=info.config;

  if (!ret->openFromUrl(url.toString())) 
  {
    VisusWarning()<<"Scene openFromUrl("<<url.toString()<<") failed";
    return SharedPtr<Scene>();
  }

  //VisusInfo()<<ret->getDatasetInfos();
  return ret; 
}

} //namespace Visus 