#include "rtreeindex.h"

#include <cfloat>

RTreeIndex::RTreeIndex() { this->root = new RTreeLeafNode(); }

RTreeNode *RTreeIndex::Insert(OGRFeature *value) {
  this->root = this->root->Insert(value);
  return this->root;
}

QVector<float> RTreeIndex::Skeleton() {
  QVector<OGREnvelope> skeleton;
  this->root->Skeleton(skeleton);
  QVector<float> vertex;
  for (OGREnvelope &envelope : skeleton) {
    vertex.append(envelope.MinX);
    vertex.append(envelope.MinY);

    vertex.append(envelope.MinX);
    vertex.append(envelope.MaxY);

    vertex.append(envelope.MaxX);
    vertex.append(envelope.MaxY);

    vertex.append(envelope.MaxX);
    vertex.append(envelope.MinY);
  }
  return vertex;
}

int RTreeBranchNode::Count() { return this->childs.count(); }

int RTreeLeafNode::Count() { return this->values.count(); }

QVector<OGREnvelope> RTreeBranchNode::SubEnvelope() {
  QVector<OGREnvelope> subs;
  for (RTreeNode *child : childs) {
    subs.append(child->envelope);
  }
  return subs;
}

QVector<OGREnvelope> RTreeLeafNode::SubEnvelope() {
  QVector<OGREnvelope> subs;
  for (OGRFeature *value : values) {
    subs.append(OGRUtils::FeatureEnvelope(value));
  }
  return subs;
}

void RTreeBranchNode::Search(const OGREnvelope &searchArea,
                             QVector<OGRFeature *> &result,
                             QVector<RTreeNode *> &node) {
  // 判断搜索区域是否与节点区域重合
  if (!envelope.Intersects(searchArea)) {
    return;
  }
  node = QVector<RTreeNode *>();
  // 如果重合，调用子节点搜索
  for (RTreeNode *child : childs) {
    child->Search(searchArea, result, node);
  }
  return;
}

void RTreeLeafNode::Search(const OGREnvelope &searchArea,
                           QVector<OGRFeature *> &result,
                           QVector<RTreeNode *> &node) {
  // 判断搜索区域是否与节点区域重合
  if (!envelope.Intersects(searchArea)) {
    return;
  }

  // 如果重合，遍历要素
  OGREnvelope envelope;
  for (OGRFeature *value : values) {
    value->GetGeometryRef()->getEnvelope(&envelope);
    // 判断搜索区域是否与要素重合
    if (envelope.Intersects(searchArea)) {
      result.append(value);
    }
  }
  node.append(this);
  return;
}

RTreeLeafNode *RTreeBranchNode::ChooseLeaf(OGRFeature *value) {
  // 找出添加E.I时，扩张最小的子节点
  // 即加入到哪个子节点中，子节点的最小外接矩形扩张最小
  double minDiff = DBL_MAX;
  RTreeNode *minChild;
  OGREnvelope valueEnvelope;
  value->GetGeometryRef()->getEnvelope(&valueEnvelope);
  for (RTreeNode *child : childs) {
    // 这里将扩张最小理解为面积增加最小
    double mergeDiffer =
        OGRUtils::EnvelopeMergeDiffer(child->envelope, valueEnvelope);
    if (mergeDiffer < minDiff) {
      minDiff = mergeDiffer;
      minChild = child;
    }
  }
  // 找到扩张最小的子节点，递归调用。递归到LeafNode为止
  return minChild->ChooseLeaf(value);
}

RTreeLeafNode *RTreeLeafNode::ChooseLeaf(OGRFeature *value) { return this; }

QVector<int> RTreeNode::pickSeed() {
  QVector<int> seed(2);
  QVector<OGREnvelope> &&subs = this->SubEnvelope();
  double maxDiff = -1;
  // 每两子区域一组，寻找扩张后面积与各自面积相差最大的一组
  for (int i = 0; i < subs.count(); i++) {
    for (int j = i + 1; j < subs.count(); j++) {
      double mergeDiffer = OGRUtils::EnvelopeMergeDiffer(subs[i], subs[j]);
      if (mergeDiffer > maxDiff) {
        maxDiff = mergeDiffer;
        // 使用[]运算符，返回引用
        seed[0] = i;
        seed[1] = j;
      }
    }
  }
  return seed;
}

QVector<QVector<int>> RTreeNode::seedGroup(QVector<int> seed) {
  QVector<QVector<int>> group(2);
  // 将种子节点添加入分组
  group[0].append(seed[0]);
  group[1].append(seed[1]);
  QVector<OGREnvelope> &&subs = this->SubEnvelope();
  for (int i = 0; i < subs.count(); i++) {
    // 如果是种子节点，不进行判断
    if (i == seed[0] || i == seed[1]) {
      continue;
    }
    // 如果不是种子节点，计算面积增量，添加到小的一组
    // 与seed1的面积增量
    double mergeDiffer1 = OGRUtils::EnvelopeMergeDiffer(subs[i], subs[seed[0]]);
    // 与seed2的面积增量
    double mergeDiffer2 = OGRUtils::EnvelopeMergeDiffer(subs[i], subs[seed[1]]);

    if (mergeDiffer1 < mergeDiffer2) {
      group[0].append(i);
    } else {
      group[1].append(i);
    }
  }
  return group;
}

RTreeNode *RTreeBranchNode::SplitNode() {
  // 创建新节点，并根据分组，设置原始节点和新节点的子节点

  QVector<QVector<int>> group = this->seedGroup(this->pickSeed());
  // 原节点的所有子节点
  QVector<RTreeNode *> childs = this->childs;
  this->childs.clear();
  RTreeBranchNode *newNode = new RTreeBranchNode();
  // 与被分裂节点具有相同的父节点
  newNode->parent = this->parent;
  for (int i = 0; i < group[0].count(); i++) {
    this->childs.append(childs[group[0][i]]);
  }
  for (int i = 0; i < group[1].count(); i++) {
    newNode->childs.append(childs[group[1][i]]);
    // 新节点需要改变子节点的父节点指针
    childs[group[1][i]]->parent = newNode;
  }
  return newNode;
}

RTreeNode *RTreeLeafNode::SplitNode() {
  // 创建新节点，并根据分组，设置原始节点和新节点的值

  QVector<QVector<int>> group = this->seedGroup(this->pickSeed());
  // 原节点的所有值
  QVector<OGRFeature *> values = this->values;
  this->values.clear();
  RTreeLeafNode *newNode = new RTreeLeafNode();
  newNode->parent = this->parent;
  for (int i = 0; i < group[0].count(); i++) {
    this->values.append(values[group[0][i]]);
  }
  for (int i = 0; i < group[1].count(); i++) {
    newNode->values.append(values[group[1][i]]);
  }
  this->AdjustEnvelope();
  newNode->AdjustEnvelope();
  return newNode;
}

void RTreeNode::AdjustEnvelope() {
  OGREnvelope currentEnvelope;
  QVector<OGREnvelope> &&subs = this->SubEnvelope();
  for (OGREnvelope sub : subs) {
    currentEnvelope.Merge(sub);
  }
  this->envelope = currentEnvelope;
}

RTreeNode *RTreeNode::AdjustTree() {
  this->AdjustEnvelope();
  if (parent == nullptr) {
    return this;
  }
  return parent->AdjustTree();
}

RTreeNode *RTreeNode::AdjustTree(RTreeNode *newNode) {
  // 根节点分裂，生成新的根节点
  if (parent == nullptr) {
    RTreeBranchNode *newRoot = new RTreeBranchNode();
    this->parent = newRoot;
    newNode->parent = newRoot;
    newRoot->childs.append(this);
    newRoot->childs.append(newNode);
    newRoot->AdjustEnvelope();
    return newRoot;
  }
  // 添加分裂节点至父节点
  parent->childs.append(newNode);
  // TODO:更换常量
  if (parent->Count() <= 4) {
    parent->AdjustEnvelope();
    return parent->AdjustTree();
  } else {
    // 父节点分裂后，可能会改变当前节点所指父节点为新分裂节点
    // 因此记录分裂前的父节点
    RTreeNode *oldParent = this->parent;
    RTreeNode *newParentNode = parent->SplitNode();
    // 父节点发生分裂，也需要添加父节点的分裂节点至其父节点
    return oldParent->AdjustTree(newParentNode);
  }
}

RTreeNode *RTreeNode::Insert(OGRFeature *value) {
  RTreeLeafNode *leafNode = ChooseLeaf(value);
  leafNode->values.append(value);
  // TODO:更换常量
  if (leafNode->values.count() > 4) {
    RTreeLeafNode *newLeafNode = (RTreeLeafNode *)leafNode->SplitNode();
    return leafNode->AdjustTree(newLeafNode);
  } else {
    return leafNode->AdjustTree();
  }
}

void RTreeBranchNode::Skeleton(QVector<OGREnvelope> &skeleton) {
  skeleton.append(this->envelope);
  for (RTreeNode *child : childs) {
    child->Skeleton(skeleton);
  }
}

void RTreeLeafNode::Skeleton(QVector<OGREnvelope> &skeleton) {
  skeleton.append(this->envelope);
  skeleton.append(this->SubEnvelope());
}
