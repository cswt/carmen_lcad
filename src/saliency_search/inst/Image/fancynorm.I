/*               -*- mode: c++ -*-

   DO NOT EDIT THIS FILE -- CHANGES WILL BE LOST!

   [Wed Dec 12 08:55:27 BRST 2012] This file was automatically generated by
   applying the script "./devscripts/extract_templates.tcl" to the template
   declarations in source file "src/Image/fancynorm.H" for types [byte,
   float]
*/
#ifndef INST_CLASS
#  define INST_CLASS
#endif
#ifdef INST_NAMESPACE
namespace INST_NAMESPACE {
#endif
template Image<byte> INST_CLASS maxNormalize(const Image<byte>& src, const byte mi, const byte ma, const MaxNormType normtyp , int nbiter , const Image<float>* lrexcit );
template Image<float> INST_CLASS maxNormalize(const Image<float>& src, const float mi, const float ma, const MaxNormType normtyp , int nbiter , const Image<float>* lrexcit );
template Image<byte> INST_CLASS maxNormalizeNone(const Image<byte>& src, const byte mi, const byte ma);
template Image<float> INST_CLASS maxNormalizeNone(const Image<float>& src, const float mi, const float ma);
template Image<byte> INST_CLASS maxNormalizeStd(const Image<byte>& src, const byte mi, const byte ma);
template Image<float> INST_CLASS maxNormalizeStd(const Image<float>& src, const float mi, const float ma);
template Image<byte> INST_CLASS maxNormalizeFancyFast(const Image<byte>& src, const byte mi, const byte ma, const int nbiter );
template Image<float> INST_CLASS maxNormalizeFancyFast(const Image<float>& src, const float mi, const float ma, const int nbiter );
template Image<byte> INST_CLASS maxNormalizeFancy(const Image<byte>& src, const byte mi, const byte ma, const int nbiter , const double weakness , const Image<float>* lrexcit );
template Image<float> INST_CLASS maxNormalizeFancy(const Image<float>& src, const float mi, const float ma, const int nbiter , const double weakness , const Image<float>* lrexcit );
template Image<byte> INST_CLASS maxNormalizeFancyLandmark(const Image<byte>& src, const byte mi, const byte ma, const int nbiter );
template Image<float> INST_CLASS maxNormalizeFancyLandmark(const Image<float>& src, const float mi, const float ma, const int nbiter );
template Image<byte> INST_CLASS maxNormalizeLandmark(const Image<byte>& src, const byte mi, const byte ma);
template Image<float> INST_CLASS maxNormalizeLandmark(const Image<float>& src, const float mi, const float ma);
template int INST_CLASS findPeaks(const Image<byte>& src, const byte mi, const byte ma, double& sum);
template int INST_CLASS findPeaks(const Image<float>& src, const float mi, const float ma, double& sum);
template float INST_CLASS goodness_map(const Image<byte>& src);
template float INST_CLASS goodness_map(const Image<float>& src);
template Image<byte> INST_CLASS maxNormalizeStdev(const Image<byte>& src);
template Image<float> INST_CLASS maxNormalizeStdev(const Image<float>& src);
template Image<byte> INST_CLASS maxNormalizeStdev0(const Image<byte>& src);
template Image<float> INST_CLASS maxNormalizeStdev0(const Image<float>& src);
#ifdef INST_NAMESPACE
}
#endif
