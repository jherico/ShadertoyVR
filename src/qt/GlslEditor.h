#pragma once

#include <QSyntaxHighlighter>

class GlslHighlighter : public QSyntaxHighlighter {
  Q_OBJECT

public:
  GlslHighlighter(bool nightMode = true, QTextDocument *parent = 0);

protected:
  void highlightBlock(const QString &text);

private:
  struct HighlightingRule {
    QRegExp pattern;
    QTextCharFormat format;
  };
  QVector<HighlightingRule> highlightingRules;

  QTextCharFormat multiLineCommentFormat;
  QRegExp commentStartExpression;
  QRegExp commentEndExpression;
};

