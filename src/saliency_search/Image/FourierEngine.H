/*!@file Image/FourierEngine.H Thin wrapper around the fftw3 library */

// //////////////////////////////////////////////////////////////////// //
// The iLab Neuromorphic Vision C++ Toolkit - Copyright (C) 2000-2005   //
// by the University of Southern California (USC) and the iLab at USC.  //
// See http://iLab.usc.edu for information about this project.          //
// //////////////////////////////////////////////////////////////////// //
// Major portions of the iLab Neuromorphic Vision Toolkit are protected //
// under the U.S. patent ``Computation of Intrinsic Perceptual Saliency //
// in Visual Environments, and Applications'' by Christof Koch and      //
// Laurent Itti, California Institute of Technology, 2001 (patent       //
// pending; application number 09/912,225 filed July 23, 2001; see      //
// http://pair.uspto.gov/cgi-bin/final/home.pl for current status).     //
// //////////////////////////////////////////////////////////////////// //
// This file is part of the iLab Neuromorphic Vision C++ Toolkit.       //
//                                                                      //
// The iLab Neuromorphic Vision C++ Toolkit is free software; you can   //
// redistribute it and/or modify it under the terms of the GNU General  //
// Public License as published by the Free Software Foundation; either  //
// version 2 of the License, or (at your option) any later version.     //
//                                                                      //
// The iLab Neuromorphic Vision C++ Toolkit is distributed in the hope  //
// that it will be useful, but WITHOUT ANY WARRANTY; without even the   //
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      //
// PURPOSE.  See the GNU General Public License for more details.       //
//                                                                      //
// You should have received a copy of the GNU General Public License    //
// along with the iLab Neuromorphic Vision C++ Toolkit; if not, write   //
// to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,   //
// Boston, MA 02111-1307 USA.                                           //
// //////////////////////////////////////////////////////////////////// //
//
// Primary maintainer for this file:
// $HeadURL: svn://isvn.usc.edu/software/invt/trunk/saliency/src/Image/FourierEngine.H $
// $Id: FourierEngine.H 7924 2007-02-15 18:13:05Z rjpeters $
//

#ifndef IMAGE_FOURIERENGINE_H_DEFINED
#define IMAGE_FOURIERENGINE_H_DEFINED

#include "Image/Image.H"

#include <complex>
#ifdef HAVE_FFTW3_H
#include <fftw3.h>
#endif

typedef std::complex<double> complexd;
typedef std::complex<float> complexf;

/// Thin wrapper around the fftw3 library for the discrete fourier transform.
/** The fftw library works by devising an optimized 'plan' for
    computing fourier transforms of arrays of a given size -- the
    'plan' is optimized at run-time for the particular machine
    architecture on which the code is being run. Therefore, it is
    necessary to use a separate FourierEngine object for each size of
    array for which one wishes to compute the fft. Typically this is
    not a significant imposition, since most applications are expected
    to compute the fft of a large number of identically-sized images
    (e.g. processing a video stream). */
template <class T>
class FourierEngine
{
public:
  /// Set up an engine for computing the fft of images with given Dims.
  FourierEngine(const Dims& d);

  /// Destructor.
  ~FourierEngine();

  /// Compute the fourier transform of x, returning the complex result.
  /** Since the full-size complex fft of a real image is symmetric,
      this function (following the fftw conventions) returns only half
      of the complex result. The full result could be reconstructed as
      needed. Also note that the result is not 'shifted' -- that is,
      the DC component will be at the upper-left corner of the
      returned image, rather than in the center. */
  Image<std::complex<T> > fft(const Image<T>& x);

private:
  FourierEngine(const FourierEngine&);
  FourierEngine& operator=(const FourierEngine&);

  const Dims       itsInputDims;
  T*               itsSrc;
  std::complex<T>* itsDst;
  void*            itsPlan;
};

/// Thin wrapper around the fftw3 for computing inverse fourier transforms
template <class T>
class FourierInvEngine
{
public:
  /// Set up an engine for computing the inverse fft.
  /** Note that the dimensions passed here should be the dimensions of
      the expected OUTPUT array, not the dimensions of the input
      array. */
  FourierInvEngine(const Dims& d);

  /// Destructor.
  ~FourierInvEngine();

  /// Compute the complex->real inverse fourier transform of x.
  /** The input is expected to be in the format/size generated by
      FourierEngine::fft(). */
  Image<T> ifft(const Image<std::complex<T> >& x);

private:
  FourierInvEngine(const FourierInvEngine&);
  FourierInvEngine& operator=(const FourierInvEngine&);

  const Dims       itsOutputDims;
  std::complex<T>* itsSrc;
  T*               itsDst;
  void*            itsPlan;
};

/// Complex magnitude.
template <class T>
Image<T> magnitude(const Image<std::complex<T> >& img);

/// Logarithm of complex magnitude.
template <class T>
Image<T> logmagnitude(const Image<std::complex<T> >& img);

/// Compute the complex phase of the fourier result.
template <class T>
Image<T> phase(const Image<std::complex<T> >& img);

/// Morph a fourier result image from polar to cartesian coordinates.
template <class T>
Image<T> cartesian(const Image<T>& in,
                   const Dims& dims);

// ##############################################################
/* So things look consistent in everyone's emacs... */
/* Local Variables: */
/* indent-tabs-mode: nil */
/* End: */

#endif // IMAGE_FOURIERENGINE_H_DEFINED
