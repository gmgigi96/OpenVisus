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

#ifndef VISUS_MATRIX_H
#define VISUS_MATRIX_H

#include <Visus/Kernel.h>
#include <Visus/Quaternion.h>
#include <Visus/Plane.h>
#include <Visus/LinearMap.h>

#include <cstring>
#include <iomanip>

namespace Visus {


///////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Matrix
{
public:

  int dim = 0;
  std::vector<double> mat;

  //default constructor
  Matrix() {
  }

  // copy constructor
  Matrix(const Matrix& other)
    : dim(other.dim), mat(other.mat) {
  }

  // constructor
  Matrix(std::vector<double> mat) {
    this->dim = (int)sqrt(mat.size());
    this->mat = mat;
    VisusAssert(this->dim * this->dim == mat.size());
  }

  // constructor 
  explicit Matrix(int dim) 
  {
    this->dim = dim;
    this->mat = std::vector<double>(dim * dim, 0.0);
    for (int I = 0; I < dim; I++) get(I, I) = 1.0;
  }

  // constructor 
  explicit Matrix(double a00, double a01, double a10, double a11)
    : Matrix(std::vector<double>({ a00,a01,a10,a11 })) {
  }

  // constructor
  explicit Matrix(double a00, double a01, double a02, double a10, double a11, double a12, double a20, double a21, double a22)
    : Matrix(std::vector<double>({ a00,a01,a02, a10,a11,a12, a20,a21,a22 })) {
  }

  //constructor for 16 doubles (row major!)
  inline explicit Matrix(
    double a00, double a01, double a02, double a03,
    double a10, double a11, double a12, double a13,
    double a20, double a21, double a22, double a23,
    double a30, double a31, double a32, double a33)
    : Matrix(std::vector<double>({
      a00,a01,a02,a03, 
      a10,a11,a12,a13, 
      a20,a21,a22,a23, 
      a30,a31,a32,a33 })) {
  }

  //constructor: transform the default axis to the system X,Y,Z in P
  inline explicit Matrix(Point3d X, Point3d Y, Point3d Z, Point3d P)
    : Matrix(
        X[0], Y[0], Z[0], P[0],
        X[1], Y[1], Z[1], P[1],
        X[2], Y[2], Z[2], P[2],
        0, 0, 0, 1) {
  }

  //constructor
  Matrix(Point3d c0, Point3d c1, Point3d c2) 
    : Matrix(
      c0[0], c1[0], c2[0],
      c0[1], c1[1], c2[1],
      c0[2], c1[2], c2[2]) {
  }

  // constructor
  static Matrix parseFromString(String s) {
    if (s.empty()) return Matrix();
    std::vector<double> v;
    double parsed;
    std::istringstream parser(s);
    while (parser >> parsed)
      v.push_back(parsed);
    return Matrix(v);
  }

  //construct from string
  static Matrix parseFromString(int pdim, String value) {
    if (value.empty()) return Matrix::identity(4);
    double parsed;
    std::vector<double> v;
    std::istringstream parser(value);
    while (parser >> parsed) v.push_back(parsed);
    VisusAssert(v.size() == pdim * pdim);
    return Matrix(v);
  }

  // destructor
  ~Matrix() {
  }

  //getSpaceDim
  int getSpaceDim() const {
    return dim;
  }

  //valid
  bool valid() const
  {
    for (int I = 0; I < 16; I++)
      if (!Utils::isValidNumber(mat[I])) return false;
    return determinant() != 0.0 ;
  }

  //isIdentity
  bool isIdentity() const {
    return *this == Matrix(getSpaceDim());
  }

  //isIdentity
  bool isZero() const {
    return *this == Matrix::zero(getSpaceDim());
  }

  // operator[]
  double& operator[](int i) {
    return this->mat[i];
  }

  //operator[]
  const double& operator[](int i) const {
    return this->mat[i];
  }

  // get
  double get(int r, int c) const {
    VisusAssert(r >= 0 && r < dim && c >= 0 && c < dim);
    return this->mat[r * dim + c];
  }

  // get
#if !SWIG
  double& get(int r, int c) {
    VisusAssert(r >= 0 && r < dim && c >= 0 && c < dim);
    return this->mat[r * dim + c];
  }
#endif

  // access operator () (stating index is 0)
  double operator()(int r, int c) const {
    return get(r, c);
  }

  // access operator () (stating index is 0)
#if !SWIG
  double& operator()(int r, int c) {
    return get(r, c);
  }
#endif

  //getRow
  PointNd getRow(int r) const
  {
    VisusAssert(r >= 0 && r < dim);
    PointNd ret(this->dim);
    for (int c = 0; c < dim; c++)
      ret[c] = get(r, c);
    return ret;
  }

  // getCol
  PointNd getCol(int c) const
  {
    VisusAssert(c >= 0 && c < dim);
    PointNd ret(this->dim);
    for (int r = 0; r < dim; r++)
      ret[r] = get(r, c);
    return ret;
  }

  // assignment operator
  Matrix& operator=(const Matrix& other)
  {
    this->dim = other.dim;
    this->mat = other.mat;
    return *this;
  }

  // operator==
  bool operator==(const Matrix& other) const {
    return this->dim == other.dim && this->mat == other.mat;
  }

  // operator!=
  bool operator!=(const Matrix& other) const {
    return !(operator==(other));
  }

  // get a zero matrix
  static Matrix zero(int dim) {
    return Matrix(std::vector<double>(dim * dim, 0.0));
  }

  // identity
  static Matrix identity(int dim) {
    return Matrix(dim);
  }

  // operator Matrix + Matrix
  Matrix operator+(const Matrix& b) const
  {
    const Matrix& a = *this;
    VisusAssert(a.dim == b.dim);
    Matrix ret(dim);
    for (int r = 0; r < dim; r++)
      for (int c = 0; c < dim; c++)
        ret(r, c) = a(r, c) + b(r, c);
    return ret;
  }

  //  operator Matrix - Matrix
  Matrix operator-(const Matrix& b) const
  {
    const Matrix& a = *this;
    VisusAssert(a.dim == b.dim);
    Matrix ret(dim);
    for (int r = 0; r < dim; r++)
      for (int c = 0; c < dim; c++)
        ret(r, c) = a(r, c) - b(r, c);
    return ret;
  }

  // operator Matrix * Matrix
  Matrix operator*(const Matrix& b) const
  {
    const Matrix& a = *this;
    VisusAssert(a.dim == b.dim);
    if (b.isIdentity()) return a;
    if (b.isIdentity()) return b;
    auto ret = Matrix::zero(dim);
    for (int i = 0; i < dim; ++i)
      for (int j = 0; j < dim; ++j)
        for (int k = 0; k < dim; ++k)
          ret(i, j) += a(i, k) * b(k, j);
    return ret;
  }

  //operator*=
  Matrix& operator*=(const Matrix& b) {
    return ((*this) = (*this) * b);
  }

  // transpose
  Matrix transpose() const
  {
    Matrix ret(dim);
    for (int i = 0; i < dim; ++i)
      for (int j = 0; j < dim; ++j)
        ret.get(i, j) = this->get(j, i);
    return ret;
  }

  // determinant
  double determinant() const;

  // invert matrix
  Matrix invert() const;

public:


  //translate
  static Matrix translate(Point2d vt) {
    return Matrix(
      1, 0, vt[0],
      0, 1, vt[1],
      0, 0, 1);
  }

  //static translate
  static Matrix translate(Point3d vt)
  {
    return Matrix(
      1, 0, 0, vt[0],
      0, 1, 0, vt[1],
      0, 0, 1, vt[2],
      0, 0, 0, 1);
  }

  // translate
  static Matrix translate(PointNd vt)
  {
    Matrix ret(vt.getPointDim() + 1);
    for (int R = 0; R < ret.dim - 1; R++)
      ret(R, ret.dim - 1) = vt[R];
    return ret;
  }


  //translate
  inline static Matrix translate(int axis, double offset) {
    Point3d vt;
    vt[axis] = offset;
    return Matrix::translate(vt);
  }

  //static scale
  static Matrix scale(Point3d vs)
  {
    return Matrix(
      vs[0], 0, 0, 0,
      0, vs[1], 0, 0,
      0, 0, vs[2], 0,
      0, 0, 0, 1);
  }

  // scale
  static Matrix scale(PointNd vs)
  {
    Matrix ret(vs.getPointDim() + 1);
    for (int I = 0; I < ret.dim - 1; I++)
      ret(I, I) = vs[I];
    return ret;
  }

  //translate
  static Matrix scale(Point2d vs) {
    return Matrix(
      vs[0], 0, 0,
      0, vs[1], 0,
      0, 0, 1);
  }

  //scaleAroundCenter
  static Matrix scaleAroundCenter(Point2d center, double vs) {
    return  Matrix::translate(center) * Matrix::scale(Point2d(vs, vs)) * Matrix::translate(-center);
  }

  //scaleAroundCenter (see http://www.gamedev.net/topic/399598-scaling-about-an-arbitrary-axis/)
  static Matrix scaleAroundAxis(Point3d axis, double k);

  //scaleAroundCenter (see http://www.gamedev.net/topic/399598-scaling-about-an-arbitrary-axis/)
  static Matrix scaleAroundCenter(Point3d center, Point3d axis, double k) {
    return  Matrix::translate(center) * scaleAroundAxis(axis, k) * Matrix::translate(-center);
  }

  //nonZeroScale
  static Matrix nonZeroScale(PointNd vs) {
    for (auto& it : vs.coords)
      it = it ? it : 1.0;
    return Matrix::scale(vs);
  }

  //invNonZeroScale
  static Matrix invNonZeroScale(PointNd vs) {
    for (auto& it : vs.coords)
      it = it ? (1.0 / it) : 1.0;
    return Matrix::scale(vs);
  }

  // rotate
  static Matrix rotate(int dim, int i, int j, double angle)
  {
    VisusAssert(i >= 0 && i < dim && j >= 0 && j < dim);
    Matrix ret(dim);
    ret(i, i) = +cos(angle); ret(i, j) = -sin(angle);
    ret(j, i) = +sin(angle); ret(j, j) = +cos(angle);
    return ret;
  }

  //rotate from quaternion
  static Matrix rotate(const Quaternion& q, Point3d vt = Point3d());

  //rotateAroundCenter
  static Matrix rotateAroundAxis(Point3d axis, double angle);

  //rotateAroundCenter
  static Matrix rotateAroundCenter(Point3d center, Point3d axis, double angle) {
    return Matrix::translate(center) * rotateAroundAxis(axis, angle) * Matrix::translate(-center);
  }

  //toQuaternion
  Quaternion toQuaternion() const;

  //static lookAt
  static Matrix lookAt(Point3d eye, Point3d center, Point3d up);

  //convert to look-at which is a triple (pos,dir,vup)
  void getLookAt(Point3d& pos, Point3d& dir, Point3d& vup) const;

  //static perspective
  static Matrix perspective(double fovy, double aspect, double zNear, double zFar);

  //static frustum
  static  Matrix frustum(double left, double right, double bottom, double top, double nearZ, double farZ);

  //static ortho
  static Matrix ortho(double left, double right, double bottom, double top, double nearZ, double farZ);

  //static viewport 
  static Matrix viewport(int x, int y, int width, int height);

  //embed (XY into a slice perpendicular to axis with certain offset)
  static Matrix embed(int axis, double offset);

  //setPointDim
  void setSpaceDim(int dim) 
  {
    if (dim == getSpaceDim())
      return;

    auto New = Matrix(dim);
    auto Old = (*this);

    int N = std::min(New.dim, Old.dim);
    int R, C;

    //copy inner avoiding right/bottom boundary
    for (R = 0; R < N - 1; R++)
      for (C = 0; C < N - 1; C++)
        New(R, C) = Old(R, C);

    //copy last column
    for (R = 0; R < N - 1; R++)
      New(R, New.dim - 1) = Old(R, Old.dim - 1);

    //copy last row
    for (C = 0; C < N - 1; C++)
      New(New.dim - 1, C) = Old(Old.dim - 1, C);

    if (N)
      New(New.dim - 1, New.dim - 1) = Old(Old.dim - 1, Old.dim - 1);

    (*this) = New;
  }

// Returns a submatrix of the current matrix, removing one row and column of the original matrix
  Matrix submatrix(int row, int column) const;

public:

  // toString
  String toString() const
  {
    std::ostringstream out;
    out << "[";
    for (int i = 0; i < this->dim; i++)
      for (int j = 0; j < this->dim; j++)
        out << (i || j ? ", " : "") << get(i, j);
    out << "]";
    return out.str();
  }

  //toStringWithPrecision
  String toStringWithPrecision(int precision = 2)
  {
    std::ostringstream o;
    for (int R = 0; R < 4; R++)
    {
      for (int C = 0; C < 4; C++)
        o << std::setprecision(precision) << std::fixed << (*this)(R, C) << " ";
      o << "\n";
    }
    return o.str();
  }

  //writeToObjectStream
  void writeToObjectStream(ObjectStream& ostream) {
    ostream.write("matrix", this->toString());
  }

  //writeToObjectStream
  void readFromObjectStream(ObjectStream& istream) {
    (*this) = Matrix::parseFromString(istream.read("matrix"));
  }

private:

  // Returns the minor of a matrix, which is the determinant of a submatrix
  double getMinor(int row, int column) const;

  //cofactor
  double cofactor(int row, int column) const;

  //cofactorMatrix
  Matrix cofactorMatrix() const;

  //adjugate
  Matrix adjugate() const;

}; //end class


#if !SWIG

inline Matrix operator*(const Matrix& T, double coeff) 
{
  Matrix ret(T.dim);
  for (int r = 0; r < T.dim; ++r)
    for (int c = 0; c < T.dim; ++c)
      ret(r, c) = T.get(r, c) * coeff;
  return ret;
}

inline Matrix operator*(double coeff, const Matrix& T) {
  return T * coeff;
}


inline PointNd operator*(const Matrix& T, PointNd p) 
{
  auto sdim = T.getSpaceDim();
  auto pdim = p.getPointDim();

  if (pdim > sdim)
    ThrowException("dimension not compatible");

  if (pdim != sdim)
  {
    p.setPointDim(sdim);
    p.back() = 1; //homogeneous
  }

  auto ret=PointNd(T.dim);
  for (int r = 0; r < T.dim; r++)
    for (int c = 0; c < T.dim; c++)
      ret[r] += T.get(r, c) * p[c];

  if (pdim !=sdim)
    ret=ret.dropHomogeneousCoordinate();

  return ret;
}

inline PointNd operator*(const PointNd& v, const Matrix& T)
{
  auto dim = v.getPointDim();
  VisusAssert(dim == T.getSpaceDim())
  PointNd ret(dim);
  for (int R = 0; R < dim; R++)
    ret += v[R] * T.getRow(R);
  return ret;
}

inline Plane operator*(const Plane& h, const Matrix& Ti) {
  return Plane(PointNd(h) * Ti);
}

inline Point3d operator*(Point3d p, const Matrix& T) {
  return (PointNd(p) * T).toPoint3();
}

inline Point3d operator*(const Matrix& T, const Point3d& p) {
  return (T * PointNd(p)).toPoint3();
}

inline Point4d operator*(const Matrix& T, const Point4d& v) {
  return (T * PointNd(v)).toPoint4();
}

inline Point4d operator*(const Point4d& v, const Matrix& T) {
  return (PointNd(v) * T).toPoint4();
}

inline Point3f operator*(const Matrix& T, const Point3f& vf) {
  return (T * vf.castTo<Point3d>()).castTo<Point3f>();
}

inline Point3d operator*(const Matrix& T, const Point3i& p) {
  return T * p.castTo<Point3d>();
}

#endif  //SWIG


//////////////////////////////////////////////////////////
class VISUS_KERNEL_API  QDUMatrixDecomposition
{
public:

  VISUS_CLASS(QDUMatrixDecomposition)

  Matrix Q = Matrix::identity(4); //rotation 
  Point3d D; //scaling
  Point3d U; //shear

  //constructor
  QDUMatrixDecomposition(const Matrix& T);

};


//////////////////////////////////////////////////////////
class VISUS_KERNEL_API TRSMatrixDecomposition
{
public:

  VISUS_CLASS(TRSMatrixDecomposition)

  //equivalent to T(translate) * R(rotate)  * S(scale) 
  Point3d    translate;
  Quaternion rotate;
  Point3d    scale;

  //constructor
  TRSMatrixDecomposition() : rotate(Point3d(0, 0, 1), 0), scale(1, 1, 1)
  {}

  //constructor 
  TRSMatrixDecomposition(const Matrix& T);

  //toMatrix
  Matrix toMatrix()
  {
    return
      Matrix::translate(translate)
      * Matrix::rotate(rotate)
      * Matrix::scale(scale);
  }


};



//////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API MatrixMap : public LinearMap
{
public:

  VISUS_CLASS(MatrixMap)

  Matrix T;
  Matrix Ti;

  //constructor
  MatrixMap() {
  }

  //constructor
  MatrixMap(const Matrix& T_) : T(T_) {
    Ti = T.invert();
  }

  //constructor
  MatrixMap(const Matrix& T_, const Matrix& Ti_) : T(T_), Ti(Ti_) {
  }

  //getSpaceDim
  int getSpaceDim() const {
    return T.getSpaceDim();
  }

  //applyDirectMap
  virtual PointNd applyDirectMap(PointNd p) const override {

    if (p.getPointDim() < getSpaceDim())
    {
      p.setPointDim(getSpaceDim());
      p.back() = 1;
    }

    return T * p;
  }

  //applyInverseMap
  virtual PointNd applyInverseMap(PointNd p) const override  {
    if (p.getPointDim() < getSpaceDim())
    {
      p.setPointDim(getSpaceDim());
      p.back() = 1;
    }
    return Ti * p;
  }

  //applyDirectMap
  virtual Plane applyDirectMap(Plane h) const override  {
    VisusAssert(h.getSpaceDim() == getSpaceDim());
    return h * Matrix(Ti);
  }

  //applyDirectMap
  virtual Plane applyInverseMap(Plane h) const override  {
    VisusAssert(h.getSpaceDim() == getSpaceDim());
    return h * Matrix(T);
  }

};


} //namespace Visus

#endif //VISUS_MATRIX_H

