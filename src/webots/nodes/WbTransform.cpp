// Copyright 1996-2024 Cyberbotics Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "WbTransform.hpp"

#include "WbBoundingSphere.hpp"
#include "WbSimulationState.hpp"
#include "WbTranslateRotateManipulator.hpp"

void WbTransform::init() {
  mScale = findSFVector3("scale");
  mPreviousXscaleValue = 1.0;
  mAbsoluteScaleNeedUpdate = true;
}

WbTransform::WbTransform(WbTokenizer *tokenizer) : WbPose("Transform", tokenizer) {
  init();
}

WbTransform::WbTransform(const WbTransform &other) : WbPose(other) {
  init();
}

WbTransform::WbTransform(const WbNode &other) : WbPose(other) {
  init();
}

void WbTransform::preFinalize() {
  WbPose::preFinalize();

  sanitizeScale();
}

void WbTransform::postFinalize() {
  WbPose::postFinalize();

  connect(mScale, SIGNAL(changed()), this, SLOT(updateScale()));
}

void WbTransform::applyToScale() {
  mBaseNode->setMatrixNeedUpdate();
  mBaseNode->setScaleNeedUpdate();

  if (mBaseNode->areWrenObjectsInitialized())
    applyScaleToWren();

  if (WbSimulationState::instance()->isRayTracingEnabled() && mBaseNode->boundingSphere())
    mBaseNode->boundingSphere()->setOwnerSizeChanged();

  if (mTranslateRotateManipulator && mTranslateRotateManipulator->isAttached())
    updateTranslateRotateHandlesSize();
}

void WbTransform::updateScale(bool warning) {
  sanitizeScale();

  applyToScale();

  if (mPoseChangedSignalEnabled)
    emit poseChanged();

  if (mHasNoSolidAncestor)
    forwardJerk();
}

void WbTransform::sanitizeScale() {
  WbVector3 sanitizedScale = mScale->value();
  bool invalid = false;

  if (sanitizedScale.x() == 0.0) {
    sanitizedScale.setX(1.0);
    mBaseNode->parsingWarn(QObject::tr("All 'scale' coordinates must be non-zero: x is set to 1.0."));
    invalid = true;
  }

  if (sanitizedScale.y() == 0.0) {
    sanitizedScale.setY(1.0);
    mBaseNode->parsingWarn(QObject::tr("All 'scale' coordinates must be non-zero: y is set to 1.0."));
    invalid = true;
  }

  if (sanitizedScale.z() == 0.0) {
    sanitizedScale.setZ(1.0);
    mBaseNode->parsingWarn(QObject::tr("All 'scale' coordinates must be non-zero: z is set to 1.0."));
    invalid = true;
  }

  if (invalid)
    mScale->setValue(sanitizedScale);
}

void WbTransform::applyToOdeScale() {
  geometry()->applyToOdeData();
}

QStringList WbTransform::fieldsToSynchronizeWithW3d() const {
  QStringList fields;
  fields << "scale" << WbPose::fieldsToSynchronizeWithW3d();
  return fields;
}

void WbTransform::updateAbsoluteScale() const {
  mAbsoluteScale = mScale->value();
  // multiply with upper transform scale if any
  const WbTransform *const up = mBaseNode->upperTransform();
  if (up)
    mAbsoluteScale *= up->absoluteScale();

  mAbsoluteScaleNeedUpdate = false;
}

const WbVector3 &WbTransform::absoluteScale() const {
  if (mAbsoluteScaleNeedUpdate)
    updateAbsoluteScale();

  return mAbsoluteScale;
}

const WbMatrix4 &WbTransform::vrmlMatrix() const {
  if (mVrmlMatrixNeedUpdate) {
    mVrmlMatrix.fromVrml(translation(), rotation(), scale());
    mVrmlMatrixNeedUpdate = false;
  }

  return mVrmlMatrix;
}

/////////////////////////
// Create WREN Objects //
/////////////////////////

void WbTransform::createWrenObjects() {
  WbPose::createWrenObjects();

  applyScaleToWren();
}

void WbTransform::setScaleNeedUpdate() {
  setScaleNeedUpdateFlag();
  WbGroup::setScaleNeedUpdate();
}

void WbTransform::applyScaleToWren() {
  float newScale[3];
  mScale->value().toFloatArray(newScale);
  wr_transform_set_scale(mBaseNode->wrenNode(), newScale);
}

///////////////////////////////////////////////////////
//  WREN methods related to WbTransform manipulators //
///////////////////////////////////////////////////////

void WbTransform::setScaleNeedUpdateFlag() const {
  // optimisation: it's useless to call the function recursively if scalarScaleNeedUpdate is true,
  // because all the children's scalarNeedUpdate are already true.
  if (mAbsoluteScaleNeedUpdate)
    return;

  mAbsoluteScaleNeedUpdate = true;
}

void WbTransform::updateMatrix() const {
  assert(mMatrix);

  // combine with upper matrix if any
  const WbPose *const pose = upperPose();
  WbVector3 t, s;
  WbRotation r;
  if (pose) {
    // to prevent shear effect in case of non-uniform scaling, it is not possible to multiply the transform matrix directly
    // note that this computation matches the one in WREN
    const WbTransform *transform = dynamic_cast<const WbTransform *>(pose);
    if (!transform)
      transform = pose->upperTransform();
    s = transform ? transform->absoluteScale() : WbVector3(1.0, 1.0, 1.0);
    WbQuaternion q = pose->rotationMatrix().toQuaternion();
    t = pose->position() + q * (s * mTranslation->value());
    mRelativeQuaternion = mRotation->value().toQuaternion();
    q = q * mRelativeQuaternion;
    q.normalize();
    r.fromQuaternion(q);
    s = absoluteScale();
  } else {
    t = mTranslation->value();
    r = mRotation->value();
    s = mScale->value();
  }

  mMatrix->fromVrml(t.x(), t.y(), t.z(), r.x(), r.y(), r.z(), r.angle(), s.x(), s.y(), s.z());
  mMatrixNeedUpdate = false;
}
