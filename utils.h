#ifndef UTILS_H
#define UTILS_H

#include "ogrsf_frmts.h"

class MatrixUtils {
 public:
  MatrixUtils();
};

class OGRUtils {
 public:
  static double EnvelopeArea(const OGREnvelope &envelope);

  static OGREnvelope FeatureEnvelope(const OGRFeature &feature);

  static OGREnvelope FeatureEnvelope(const OGRFeature *const feature);

  // 计算两个MBR合并之后的面积增量（面积扩张量）
  static double EnvelopeMergeDiffer(const OGREnvelope &envelope1,
                                    const OGREnvelope &envelope2);
};

#endif  // UTILS_H
