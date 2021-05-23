#include "utils.h"

MatrixUtils::MatrixUtils() {}

double OGRUtils::EnvelopeArea(const OGREnvelope &envelope) {
  return (envelope.MaxX - envelope.MinX) * (envelope.MaxY - envelope.MinY);
}

OGREnvelope OGRUtils::FeatureEnvelope(const OGRFeature &feature) {
  OGREnvelope envelope;
  feature.GetGeometryRef()->getEnvelope(&envelope);
  return envelope;
}

OGREnvelope OGRUtils::FeatureEnvelope(const OGRFeature *const feature) {
  OGREnvelope envelope;
  feature->GetGeometryRef()->getEnvelope(&envelope);
  return envelope;
}

double OGRUtils::EnvelopeMergeDiffer(const OGREnvelope &envelope1,
                                     const OGREnvelope &envelope2) {
  OGREnvelope mergeEnvelope;
  mergeEnvelope.Merge(envelope1);
  mergeEnvelope.Merge(envelope2);
  return EnvelopeArea(mergeEnvelope) - EnvelopeArea(envelope1) -
         EnvelopeArea(envelope2);
}
