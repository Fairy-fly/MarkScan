#pragma once
#include <QColor>
#include <QIcon>
class QApplication;
namespace theme {
inline constexpr auto Background="#F3F2EE";
inline constexpr auto Surface="#FFFFFF";
inline constexpr auto Ink="#252833";
inline constexpr auto Muted="#727580";
inline constexpr auto Line="#DDDED9";
inline constexpr auto Accent="#535D84";
inline constexpr auto Correct="#30705A";
inline constexpr auto Wrong="#AC4845";
inline constexpr auto Unanswered="#787B85";
inline constexpr auto Multiple="#A36B2E";
inline constexpr int Touch=48, Radius=10, Gap=16, Margin=20, QuestionHeight=76;
void apply(QApplication& app,bool mobile);
QIcon icon(const QString& name,const QColor& color=QColor(Ink));
}
