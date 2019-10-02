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

#ifndef VISUS_TRANSFER_FUNCTION_H
#define VISUS_TRANSFER_FUNCTION_H

#include <Visus/Kernel.h>
#include <Visus/Model.h>
#include <Visus/Color.h>
#include <Visus/Array.h>

namespace Visus {


//////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API SingleTransferFunction
{
public:

  VISUS_CLASS(SingleTransferFunction)

  String              name;
  Color               color;
  std::vector<double> values;

  //constructor
  SingleTransferFunction() {
  }

  //constructor (identity function)
  SingleTransferFunction(String name_, Color color_ = Colors::Black, std::vector<double> values_=std::vector<double>(256,0.0))
    : name(name_),color(color_),values(values_) {
  }

  //destructor
  virtual ~SingleTransferFunction() {
  }

  //getNumberOfSamples
  inline int getNumberOfSamples() const {
    return (int)values.size();
  }

  //setNumberOfSamples
  void setNumberOfSamples(int value) {
    std::vector<double> values(value);
    for (int I = 0; I < value; I++)
      values[I] = getValue(I / (double)(value - 1));
    this->values = values;
  }

  //getValue (x must be in range [0,1])
  double getValue(double x) const
  {
    int N = getNumberOfSamples();
    if (!N) {
      VisusAssert(false);
      return 0;
    }

    x = Utils::clamp(x * (N - 1), 0.0, N - 1.0);

    int i_x1 = Utils::clamp((int)std::floor(x), 0, N - 1);
    int i_x2 = Utils::clamp((int)std::ceil(x), 0, N - 1);

    if (i_x1 == i_x2)
      return values[(int)i_x1];

    double alpha = (i_x2 - x) / (double)(i_x2 - i_x1);
    double beta = 1 - alpha;
    return alpha * values[i_x1] + beta * values[i_x2];
  }

public:

  //writeTo
  void writeTo(StringTree& out) const
  {
    std::ostringstream ss;
    for (int I = 0; I < (int)this->values.size(); I++)
    {
      if (I % 16 == 0) ss << std::endl;
      ss << this->values[I] << " ";
    }

    out.writeValue("name", name);
    out.writeValue("color", color.toString());
    out.writeText("values", ss.str());
  }

  //readFrom
  void readFrom(StringTree& in)
  {
    name = in.readValue("name");
    color = Color::fromString(in.readValue("color"));

    this->values.clear();

    std::istringstream ss(in.readText("values"));
    double value;
    while (ss >> value)
      this->values.push_back(value);
  }

  //encode
  StringTree encode(String root_name) const {
    StringTree out(root_name);
    writeTo(out);
    return out;
  }

  //decode
  void decode(StringTree in) {
    readFrom(in);
  }

};


////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API TransferFunction : public Model
{
public:

  //functions
  std::vector< SharedPtr<SingleTransferFunction> > functions;

  //texture
  SharedPtr<Object> texture;

  //constructor
  TransferFunction() {
  }

  //copy constructor
  TransferFunction(const TransferFunction& other) {
    operator=(other);
  }

  //destructor
  virtual ~TransferFunction() {
  }

  //fromArray
  static SharedPtr<TransferFunction> fromArray(Array src);

  //fromString
  static SharedPtr<TransferFunction> fromString(String content);

  //getDefault
  static SharedPtr<TransferFunction> getDefault(String default_name, const int nsamples=256);

  //guessName
  static String guessName(int I) {
    return std::vector<String>({ "Red","Green","Blue","Alpha",cstring(I) })[Utils::clamp(I, 0, 4)];
  }

  //guessColor
  static Color guessColor(int I) {
    return std::vector<Color>({ Colors::Red, Colors::Green, Colors::Blue, Colors::Gray,Color::random() })[Utils::clamp(I, 0, 4)];
  }

  //getTypeName
  virtual String getTypeName() const override {
    return "TransferFunction";
  }

  //getNumberOfSamples
  int getNumberOfSamples() const {
    return functions.empty() ? 0 : functions[0]->getNumberOfSamples();
  }

  //setNumberOfSamples
  void setNumberOfSamples(int value);

  //getNumberOfFunctions
  int getNumberOfFunctions() const {
    return (int)functions.size();
  }

  //setNumberOfFunctions
  void setNumberOfFunctions(int value);

  //valid
  bool valid() const {
    return getNumberOfSamples() > 0 && getNumberOfFunctions() >0;
  }

  //executeAction
  virtual void executeAction(StringTree in) override;

  //operator=
  TransferFunction& operator=(const TransferFunction& other);

  //isDefault
  bool isDefault() const {
    return !default_name.empty();
  }

  //getDefaultName
  String getDefaultName() const {
    return default_name;
  }

  //getAttenuation
  double getAttenuation() const {
    return attenuation;
  }

  //setAttenutation
  void setAttenutation(double value) {
    setProperty("attenuation", value, this->attenuation);
  }

  //getInputRange
  ComputeRange getInputRange() const {
    return input_range;
  }

  //setInputRange
  void setInputRange(ComputeRange new_value);

  //getOutputDType
  DType getOutputDType() const {
    return output_dtype;
  }

  //setOutputDType
  void setOutputDType(DType value) {
    setProperty("output_dtype", value, this->output_dtype);
  }

  //getOutputRange
  Range getOutputRange() const {
    return output_range;
  }

  //setOutputRange
  void setOutputRange(Range value) {
    setProperty("output_range", value, this->output_range);
  }

  //clearFunctions
  void clearFunctions();

  //addFunction
  void addFunction(SharedPtr<SingleTransferFunction> fn);

  //removeFunction
  void removeFunction(int index);

  //getDefaults
  static std::vector<String> getDefaults();

  //drawLine (x and y in range [0,1])
  void drawLine(Point2d p1, Point2d p2, std::vector<int> selected);

public:

  //toArray
  Array toArray() const;

public:

  //importTransferFunction
  static TransferFunction importTransferFunction(String content);

  //exportTransferFunction
  bool exportTransferFunction(String filename);

public:

  //writeTo
  virtual void writeTo(StringTree& out) const override;

  //readFrom
  virtual void readFrom(StringTree& in) override;

private:

  //default_name
  String default_name;

  //see https://github.com/sci-visus/visus-issues/issues/260
  double attenuation = 0.0;

  //input_range
  ComputeRange input_range;

  //what is the output (this must be atomic)
  DType output_dtype = DTypes::UINT8;

  //how to map the range [0,1] to some user range
  Range output_range = Range(0, 255, 1);

};

typedef TransferFunction Palette;

} //namespace Visus

#endif //VISUS_TRANSFER_FUNCTION_H

