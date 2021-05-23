#ifndef RTREEINDEX_H
#define RTREEINDEX_H

#include <QVector>

#include "ogrsf_frmts.h"
#include "utils.h"

class RTreeBranchNode;

class RTreeLeafNode;

/*
节点：
    分支节点：指向分支节点或者叶节点
    叶节点：指向要素
*/
class RTreeNode {
 public:
  // 父节点
  RTreeBranchNode *parent = nullptr;

  virtual int Count() = 0;

  //    QVector<RTreeNode*> childs;
  //    QVector<OGRFeature*> values;

  // 每个节点都有MBR，表示节点区域、外接矩形
  OGREnvelope envelope;

  // 按照子节点或要素存储的顺序，返回它们的MBR
  virtual QVector<OGREnvelope> SubEnvelope() = 0;

  // 使用虚函数时，要注意虚函数的虚调用与实调用
  virtual void Search(const OGREnvelope &searchArea,
                      QVector<OGRFeature *> &result,
                      QVector<RTreeNode *> &node) = 0;

  // 获取要素插入的叶结点。依据扩张最小原则
  virtual RTreeLeafNode *ChooseLeaf(OGRFeature *value) = 0;

  // 使用Quadratic（二次方）方案，挑选分裂的两个种子节点。尽量不重叠原则
  QVector<int> pickSeed();

  // 根据挑选的种子节点，返回分组
  QVector<QVector<int>> seedGroup(QVector<int> seed);

  // 分裂节点，返回分裂的新节点
  virtual RTreeNode *SplitNode() = 0;

  void AdjustEnvelope();

  // 调整树，返回根节点。无参只做MBR调整
  RTreeNode *AdjustTree();

  // 有参则添加新的分裂节点至父节点
  RTreeNode *AdjustTree(RTreeNode *newNode);

  // 利用辅助方法，插入要素
  RTreeNode *Insert(OGRFeature *value);

  // 获取所有节点的MBR
  virtual void Skeleton(QVector<OGREnvelope> &skeleton) = 0;
};

class RTreeBranchNode : public RTreeNode {
 public:
  // 分支节点具有子节点，子节点可能是分支节点或叶节点
  QVector<RTreeNode *> childs;

  int Count();

  QVector<OGREnvelope> SubEnvelope() override;

  void Search(const OGREnvelope &searchArea, QVector<OGRFeature *> &result,
              QVector<RTreeNode *> &node) override;

  RTreeLeafNode *ChooseLeaf(OGRFeature *value) override;

  RTreeNode *SplitNode() override;

  void Skeleton(QVector<OGREnvelope> &skeleton) override;
};

class RTreeLeafNode : public RTreeNode {
 public:
  // 叶节点没有子节点，但是它包含要素
  QVector<OGRFeature *> values;

  int Count();

  QVector<OGREnvelope> SubEnvelope() override;

  void Search(const OGREnvelope &searchArea, QVector<OGRFeature *> &result,
              QVector<RTreeNode *> &node) override;

  RTreeLeafNode *ChooseLeaf(OGRFeature *value) override;

  RTreeNode *SplitNode() override;

  void Skeleton(QVector<OGREnvelope> &skeleton) override;
};

class RTreeIndex {
 public:
  RTreeIndex();

  // 根节点。初始化时为叶结点，分裂后变为分支节点
  RTreeNode *root = nullptr;

  RTreeNode *Insert(OGRFeature *value);

  // 根据所有节点的MBR，生成节点数组
  QVector<float> Skeleton();
};

#endif  // RTREEINDEX_H
