%module(directors="1") VisusKernelPy 

%{ 
#define SWIG_FILE_WITH_INIT
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>
#include <Visus/Array.h>
#include <Visus/Frustum.h>
#include <Visus/Time.h>
#include <Visus/VisusConfig.h>
#include <Visus/Model.h>
#include <Visus/Semaphore.h>
using namespace Visus;
%}

 
%include <VisusSwigCommon.i>

//swig get confused between Visus::Exception and python Exception class, so better to rename it
%rename(CppException) Visus::Exception;

// new object
%newobject Visus::ObjectStream::readObject;
%newobject Visus::ObjectCreator::createInstance;
%newobject Visus::ObjectFactory::createInstance;
//%newobject Visus::ScopedVector::release;
%newobject Visus::StringTreeEncoder::encode;
%newobject Visus::StringTreeEncoder::decode;
%newobject Visus::CloudStorage::createInstance;
%newobject Visus::CloudStorage::createInstance;

// SharedPtr
//grep -E -r -i -o -h --include *.h "SharedPtr<([[:space:]])*([[:alnum:]]+)([[:space:]]*)>" src  | sort -u
ENABLE_SHARED_PTR(Array)
ENABLE_SHARED_PTR(ArrayPlugin)
//ENABLE_SHARED_PTR(bool)
%template(BoolPtr) Visus::SharedPtr<bool>;

ENABLE_SHARED_PTR(DictObject)
ENABLE_SHARED_PTR(Frustum)
ENABLE_SHARED_PTR(HeapMemory)
ENABLE_SHARED_PTR(Object)
ENABLE_SHARED_PTR(ObjectCreator)
ENABLE_SHARED_PTR(Semaphore)
ENABLE_SHARED_PTR(StringTree)

// disown
// NOTE Ignoring ScopedVector which should not be exposed:
// Visus::ScopedVector::*(...VISUS_DISOWN....)

#if NUMPY_FOUND
%init %{
  import_array();
%}
#endif

%include <Visus/Visus.h>

//______________________________ Kernel/Core
%include <Visus/Platform.h>
%include <Visus/Kernel.h>
%include <Visus/Time.h>
%include <Visus/StringMap.h>
%include <Visus/StringUtils.h>
%include <Visus/Url.h>
%include <Visus/CriticalSection.h>
%include <Visus/Exception.h>
%include <Visus/Log.h>
%include <Visus/HeapMemory.h>
%include <Visus/Singleton.h>
%include <Visus/Object.h>
%include <Visus/Aborted.h>
%include <Visus/Semaphore.h>
%include <Visus/Async.h>
%include <Visus/StringTree.h>
%include <Visus/Utils.h>
%include <Visus/VisusConfig.h>
%include <Visus/SignalSlot.h>
  %ignore Visus::Model::begin_update;
  %ignore Visus::Model::changed;
  %ignore Visus::Model::destroyed;
%include <Visus/Model.h>

%include <Visus/Point.h> 
  %template(Point2i)    Visus::Point2<int   > ;
  %template(Point2f)    Visus::Point2<float > ;
  %template(Point2d)    Visus::Point2<double> ;
  %template(Point3i)    Visus::Point3<int  >  ;
  %template(Point3f)    Visus::Point3<float>  ;
  %template(Point3d)    Visus::Point3<double> ;
  %template(Point4i)    Visus::Point4<int   > ;
  %template(Point4f)    Visus::Point4<float > ;
  %template(Point4d)    Visus::Point4<double> ;
  %template(PointNi)    Visus::PointN<int >   ;
  %template(PointNf)    Visus::PointN<float > ;
  %template(PointNd)    Visus::PointN<double> ;
  %template(NdPoint)    Visus::PointN< Visus::Int64 > ;
%include <Visus/Plane.h>
%include <Visus/Box.h>
  %template(Box3d)    Visus::Box3<double> ;
  %template(Box3i)    Visus::Box3<int> ;
  %template(BoxNd)    Visus::BoxN<double> ;
  %template(NdBox)    Visus::BoxN< Visus::Int64 >;


%include <Visus/Line.h>
%include <Visus/LinearMap.h>
%include <Visus/Quaternion.h>
%include <Visus/Rectangle.h>
%include <Visus/Matrix.h>
%include <Visus/Position.h>
%include <Visus/Frustum.h> 
%include <Visus/Range.h>
%include <Visus/DType.h>
%template(VectorOfField) std::vector<Visus::Field>;
%include <Visus/Field.h>

%template(VectorOfArray) std::vector<Visus::Array>;
%include <Visus/Array.h>

%extend Visus::Array {

  //operator[]
  Visus::Array operator[](int index) const {return $self->getComponent(index);}
  Visus::Array operator+(Visus::Array& other) const {return ArrayUtils::add(*self,other);}
  Visus::Array operator-(Visus::Array& other) const {return ArrayUtils::sub(*self,other);}
  Visus::Array operator*(Visus::Array& other) const {return ArrayUtils::mul(*self,other);}
  Visus::Array operator*(double coeff) const {return ArrayUtils::mul(*self,coeff);}
  Visus::Array operator/(Visus::Array& other) const {return ArrayUtils::div(*self,other);}
  Visus::Array operator/(double coeff) const {return ArrayUtils::div(*self,coeff);}
  Visus::Array& operator+=(Visus::Array& other)  {*self=ArrayUtils::add(*self,other); return *self;}
  Visus::Array& operator-=(Visus::Array& other)  {*self=ArrayUtils::sub(*self,other); return *self;}
  Visus::Array& operator*=(Visus::Array& other)  {*self=ArrayUtils::mul(*self,other); return *self;}
  Visus::Array& operator*=(double coeff) {*self=ArrayUtils::mul(*self,coeff); return *self;}
  Visus::Array& operator/=(Visus::Array& other)  {*self=ArrayUtils::div(*self,other); return *self;} 
  Visus::Array& operator/=(double coeff)  {*self=ArrayUtils::div(*self,coeff); return *self;}

  static Visus::Array fromVectorInt32(Visus::NdPoint dims, const std::vector<Visus::Int32>& vector) {
	return Visus::Array::fromVector<Visus::Int32>(dims, Visus::DTypes::INT32, vector);
  }

  static Visus::Array fromVectorFloat64(Visus::NdPoint dims, const std::vector<Visus::Float64>& vector) {
    return Visus::Array::fromVector<Visus::Float64>(dims, Visus::DTypes::FLOAT64, vector);
  }


  %pythoncode %{
   
    def fromPyArray(array) :

      def getDimensions(array):
        if not type(array) is list: 
          return []
        else:
          dims=getDimensions(array[0])
          dims.append(len(array))
          return dims

      def flatten(array):
        if not type(array) is list:
          return [array]
        ret=[]
        for it in array:
          ret.extend(flatten(it)) 
        return ret

      dims=getDimensions(array)
      pdim=len(dims)
      if   (pdim==1): dims=NdPoint.one(1).withX(dims[0])
      elif (pdim==2): dims=NdPoint.one(dims[0],dims[1])
      elif (pdim==3): dims=NdPoint.one(dims[0],dims[1],dims[2])
      else: raise Exception("internal error")

      vector=flatten(array)
      if (len(vector)!=dims.innerProduct()):
        raise Exception("wrong dimension")

      if (isinstance(vector[0],int)):
        return Array.fromVectorInt32(dims,vector)

      if (isinstance(vector[0],float)):
        return Array.fromVectorFloat64(dims,vector)

      raise Exception("internal error")

    fromPyArray = staticmethod(fromPyArray)

    def __rmul__(self, v):
      return self.__mul__(v)

  %}
};

#if NUMPY_FOUND

//Array <-> numpy
%extend Visus::Array 
{
  //do not use directly!
  PyObject* internalConvertToNumPyArray() 
  {
    //in numpy the first dimension is the "upper dimension"
    //example:
    // a=array([[1,2,3],[4,5,6]])
    // print a.shape # return 2,3
    // print a[1,1]  # equivalent to print a[Y,X], return 5  
    npy_intp shape[VISUS_NDPOINT_DIM+1]; int shape_dim=0;
    for (int I=VISUS_NDPOINT_DIM-1;I>=0;I--) {
    if ($self->dims[I]>1) 
      shape[shape_dim++]=(npy_int)$self->dims[I];
    }

    int   ndtype        = $self->dtype.ncomponents();
    DType single_dtype  = $self->dtype.get(0);
    if (ndtype>1) shape[shape_dim++]=(npy_int)ndtype;
    int typenum;
    if      (single_dtype==(DTypes::UINT8  )) typenum=NPY_UINT8 ;
    else if (single_dtype==(DTypes::INT8   )) typenum=NPY_INT8  ;
    else if (single_dtype==(DTypes::UINT16 )) typenum=NPY_UINT16;
    else if (single_dtype==(DTypes::INT16  )) typenum=NPY_INT16 ;
    else if (single_dtype==(DTypes::UINT32 )) typenum=NPY_UINT32;
    else if (single_dtype==(DTypes::INT32  )) typenum=NPY_INT32 ;
    else if (single_dtype==(DTypes::UINT64 )) typenum=NPY_UINT64;
    else if (single_dtype==(DTypes::INT64  )) typenum=NPY_INT64 ;
    else if (single_dtype==(DTypes::FLOAT32)) typenum=NPY_FLOAT ;
    else if (single_dtype==(DTypes::FLOAT64)) typenum=NPY_DOUBLE;
    else {VisusInfo()<<"numpy type not supported "<<$self->dtype.toString();return NULL;}
    return PyArray_SimpleNewFromData(shape_dim,shape,typenum,$self->c_ptr());
  }
  

  //do not use directly!
  static SharedPtr<Visus::Array> internalConvertToVisusArray(PyObject* obj)
  {
    if (!((obj) && PyArray_Check((PyArrayObject *)obj))) {
      SWIG_SetErrorMsg(PyExc_NotImplementedError,"input argument is not a numpy array\n");
      return SharedPtr<Visus::Array>();
    }
    
    PyArrayObject* array = (PyArrayObject*) obj;
    if (!PyArray_ISCONTIGUOUS(array)) {
      SWIG_SetErrorMsg(PyExc_NotImplementedError,"numpy array is null or not contiguous\n");
      return SharedPtr<Visus::Array>();
    }

    Uint8* c_ptr=(Uint8*)array->data;
    
    NdPoint dims=NdPoint::one(array->nd);
    for (int I=0;I<array->nd;I++)
      dims[I]=array->dimensions[array->nd-1-I];

    DType dtype;
    if      (PyArray_TYPE(array)==NPY_UINT8  ) dtype=(DTypes::UINT8  );
    else if (PyArray_TYPE(array)==NPY_INT8   ) dtype=(DTypes::INT8   );
    else if (PyArray_TYPE(array)==NPY_UINT16 ) dtype=(DTypes::UINT16 );
    else if (PyArray_TYPE(array)==NPY_INT16  ) dtype=(DTypes::INT16  );
    else if (PyArray_TYPE(array)==NPY_UINT32 ) dtype=(DTypes::UINT32 );
    else if (PyArray_TYPE(array)==NPY_INT32  ) dtype=(DTypes::INT32  );
    else if (PyArray_TYPE(array)==NPY_UINT64 ) dtype=(DTypes::UINT64 );
    else if (PyArray_TYPE(array)==NPY_INT64  ) dtype=(DTypes::INT64  );
    else if (PyArray_TYPE(array)==NPY_FLOAT  ) dtype=(DTypes::FLOAT32);
    else if (PyArray_TYPE(array)==NPY_DOUBLE ) dtype=(DTypes::FLOAT64);
    else {
      SWIG_SetErrorMsg(PyExc_NotImplementedError,"cannot guess visus dtype from numpy array_type\n");
      return SharedPtr<Visus::Array>();
    }

    return std::make_shared<Visus::Array>(dims,dtype,SharedPtr<HeapMemory>(HeapMemory::createUnmanaged(c_ptr,dtype.getByteSize(dims))));
  }
}

%pythoncode %{

# here I'm sharing the heap memory, so I need to make sure that if one gets destroyed it won't crash (see __keep_in_memory__)

import numpy

def convertToNumPyArray(visus_value):

  # see https://docs.scipy.org/doc/numpy-1.10.1/user/basics.subclassing.html 
  # section "Simple example - adding an extra attribute to ndarray""
  class NumPyArrayWithKeepInMemory(numpy.ndarray):

    def __new__(subtype, shape, dtype=float, buffer=None, offset=0,strides=None, order=None, __keep_in_memory__=None):

      obj = numpy.ndarray.__new__(subtype, shape, dtype, buffer, offset, strides,order)
      obj.__keep_in_memory__ = __keep_in_memory__
      return obj

    def __array_finalize__(self, obj):
      if obj is None: return
      self.__keep_in_memory__ = getattr(obj, '__keep_in_memory__', None)

  numpy_value=visus_value.internalConvertToNumPyArray().view(NumPyArrayWithKeepInMemory)
  numpy_value.__keep_in_memory__ = visus_value.heap 
  return numpy_value

def convertToVisusArray(numpy_value):
  visus_value=Array.internalConvertToVisusArray(numpy_value)
  visus_value.__keep_in_memory__=numpy_value 
  return visus_value
%}

#endif 

%include <Visus/Color.h>

//disabled code, I don't think I need to expose these classes
#if 0
 
ENABLE_SHARED_PTR(Action)
ENABLE_SHARED_PTR(CloudStorage)
ENABLE_SHARED_PTR(Connection)
ENABLE_SHARED_PTR(Encoder)
ENABLE_SHARED_PTR(FGraph)
ENABLE_SHARED_PTR(File)
ENABLE_SHARED_PTR(KdArray)
ENABLE_SHARED_PTR(KdArrayNode)
ENABLE_SHARED_PTR(NetConnection)
ENABLE_SHARED_PTR(NetRequest)
ENABLE_SHARED_PTR(NetServerModule)
ENABLE_SHARED_PTR(NetService)
ENABLE_SHARED_PTR(NetSocket)
ENABLE_SHARED_PTR(PythonEngine)
ENABLE_SHARED_PTR(ThreadPool)
ENABLE_SHARED_PTR(SingleTransferFunction)
ENABLE_SHARED_PTR(TransferFunction)
ENABLE_SHARED_PTR(XmlEncoder)

%include <Visus/AmazonCloudStorage.h>
%include <Visus/ApplicationInfo.h>
%include <Visus/ApplicationStats.h>
%include <Visus/AzureCloudStorage.h>
%include <Visus/BigInt.h>
%include <Visus/Circle.h>
%include <Visus/CloudStorage.h>
%include <Visus/ConvexHull.h>
%include <Visus/Diff.h>
%include <Visus/DirectoryIterator.h>
%include <Visus/Encoders.h>
%include <Visus/File.h>
%include <Visus/FindRoots.h>
%include <Visus/Graph.h>
%include <Visus/Histogram.h>
%include <Visus/KdArray.h>
%include <Visus/LocalCoordinateSystem.h>
%include <Visus/NetMessage.h>
%include <Visus/NetServer.h>
%include <Visus/NetService.h>
%include <Visus/NetSocket.h>
%include <Visus/NumericLimits.h>
%include <Visus/Path.h>
%include <Visus/PointCloud.h>
%include <Visus/Polygon.h>
#include <Visus/PythonEngine.h>
%include <Visus/RamResource.h>
%include <Visus/Ray.h>
%include <Visus/ScopedVector.h>
%include <Visus/Segment.h>
%include <Visus/Sphere.h>
%include <Visus/Statistics.h>
%include <Visus/Thread.h>
%include <Visus/ThreadPool.h>
%include <Visus/TransferFunction.h>
%include <Visus/UnionFind.h>
%include <Visus/UUID.h>

#endif

%pythoncode %{

# equivalent to VISUS_REGISTER_OBJECT_CLASS 
def VISUS_REGISTER_PYTHON_OBJECT_CLASS(object_name):

  class MyObjectCreator(ObjectCreator):

    # __init__
    def __init__(self,object_name):
      ObjectCreator.__init__(self)
      self.object_name=object_name

    # createInstance
    def createInstance(self):
      return eval(self.object_name+"()")

  ObjectFactory.getSingleton().registerObjectClass(object_name,object_name,MyObjectCreator(object_name))
      


%}