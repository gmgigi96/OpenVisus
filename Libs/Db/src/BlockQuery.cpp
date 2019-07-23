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

#include <Visus/BlockQuery.h>
#include <Visus/Dataset.h>

namespace Visus {

////////////////////////////////////////////////////////////////////////////////////
BlockQuery::BlockQuery(Dataset* dataset_, Field field_, double time_, BigInt start_address_, BigInt end_address_, int mode_,Aborted aborted_)
  : Query(dataset_,field_,time_, mode_, aborted_), start_address(start_address_), end_address(end_address_)
{
 
  this->logic_box = dataset->getAddressRangeBox(this->start_address, this->end_address);
}

////////////////////////////////////////////////////////////////////////////////////
bool Query::allocateBufferIfNeeded()
{
  auto nsamples = getNumberOfSamples();

  if (!buffer)
  {
    if (!buffer.resize(nsamples, field.dtype, __FILE__, __LINE__))
      return false;

    buffer.fillWithValue(field.default_value);
    buffer.layout = field.default_layout;
  }
  else
  {
    //check buffer
    VisusAssert(buffer.dims == nsamples);
    VisusAssert(buffer.dtype == field.dtype);
    VisusAssert(buffer.c_size() == getByteSize());
  }

  return true;
}

} //namespace Visus



