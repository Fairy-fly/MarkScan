#include "desktop/MainWindow.h"
#include "desktop/Theme.h"
#include "desktop/ImageCanvas.h"
#include "desktop/ScoreCard.h"
#include "android/Storage.h"
#include <QtConcurrent/QtConcurrentRun>
#include <QApplication>
#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QButtonGroup>
#include <QProgressBar>
#include <QSaveFile>
#include <QScrollArea>
#include <QScrollBar>
#include <QScroller>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>
#include <QEventLoop>
#include <QDebug>
#include <opencv2/imgproc.hpp>
namespace {
QImage toImage(const cv::Mat&m){if(m.empty())return {};return QImage(m.data,m.cols,m.rows,int(m.step),QImage::Format_BGR888).copy();}
QString statusText(markscan::Status s){using markscan::Status;switch(s){case Status::Correct:return "正确";case Status::Wrong:return "错误";case Status::Multiple:return "多选";default:return "漏选";}}
QColor statusColor(markscan::Status s){using markscan::Status;switch(s){case Status::Correct:return QColor(theme::Correct);case Status::Wrong:return QColor(theme::Wrong);case Status::Multiple:return QColor(theme::Multiple);default:return QColor(theme::Unanswered);}}
bool writeJson(const QString&p,const QJsonObject&o){QSaveFile f(p);if(!f.open(QIODevice::WriteOnly))return false;auto d=QJsonDocument(o).toJson();return f.write(d)==d.size()&&f.commit();}
QLabel*label(const QString&t,const char*r=nullptr){auto l=new QLabel(t);l->setWordWrap(true);if(r)l->setProperty("role",r);return l;}
QPushButton*button(const QString&t,const char*r=nullptr){auto b=new QPushButton(t);if(r)b->setProperty("role",r);b->setCursor(Qt::PointingHandCursor);b->setMinimumHeight(theme::Touch);return b;}
QFrame*divider(){auto w=new QFrame;w->setObjectName("divider");w->setFixedHeight(1);return w;}
QPixmap illustration(){
 QPixmap pix(600,460);pix.fill(Qt::transparent);QPainter p(&pix);p.setRenderHint(QPainter::Antialiasing);p.scale(2,2);
 p.setPen(Qt::NoPen);p.setBrush(QColor("#E2E4DE"));p.drawEllipse(QRectF(38,18,224,200));p.setBrush(QColor("#FBFAF6"));p.setPen(QPen(QColor("#CED1CC"),1));p.drawRoundedRect(QRectF(92,18,126,184),6,6);
 p.setPen(Qt::NoPen);p.setBrush(QColor(theme::Accent));for(auto a:{QPoint(104,30),QPoint(198,30),QPoint(104,182),QPoint(198,182)})p.drawRect(QRect(a,QSize(7,7)));
 p.setPen(QPen(QColor("#A6ACA7"),1));for(int q=0;q<5;++q)for(int a=0;a<4;++a)p.drawEllipse(QPointF(120+a*22,76+q*23),5,5);
 p.setBrush(QColor(theme::Accent));p.setPen(Qt::NoPen);for(int q=0;q<5;++q)p.drawEllipse(QPointF(120+(q%4)*22,76+q*23),4,4);
 p.setPen(QPen(QColor(theme::Accent),2,Qt::SolidLine,Qt::RoundCap));p.drawLine(126,49,183,49);p.setBrush(QColor("#D4DDD3"));p.setPen(Qt::NoPen);p.drawEllipse(QPointF(226,169),26,26);p.setPen(QPen(QColor(theme::Correct),2.5,Qt::SolidLine,Qt::RoundCap));p.drawLine(215,169,223,177);p.drawLine(223,177,238,160);return pix;
}
}
MainWindow::MainWindow(bool mobile):mobile_(mobile){
 setWindowTitle("MarkScan · 答题卡评分");watcher_=new QFutureWatcher<markscan::Result>(this);
 auto central=new QWidget;auto root=new QVBoxLayout(central);root->setContentsMargins(0,0,0,0);root->setSpacing(0);setCentralWidget(central);
 auto header=new QWidget;auto h=new QHBoxLayout(header);h->setContentsMargins(20,mobile_?8:12,20,8);h->setSpacing(10);
 auto mark=new QLabel;mark->setPixmap(theme::icon("brand",QColor(theme::Accent)).pixmap(28,28));h->addWidget(mark);
 auto branding=new QVBoxLayout;branding->setSpacing(0);branding->addWidget(label("MarkScan","brand"));branding->addWidget(label("答题卡评分工作台","muted"));h->addLayout(branding);h->addStretch();
 if(!mobile_){h->addWidget(label("本地处理 · 固定模板","muted"));h->addSpacing(18);}
 more_=button("","quiet");more_->setIcon(theme::icon("more"));more_->setIconSize({24,24});more_->setFixedWidth(48);more_->setAccessibleName("更多");more_->setToolTip("更多");h->addWidget(more_);root->addWidget(header);root->addWidget(divider());
 screens_=new QStackedWidget;root->addWidget(screens_,1);screens_->addWidget(buildEmpty());screens_->addWidget(buildBusy());screens_->addWidget(buildResults());screens_->addWidget(buildFailure());
 feedback_=label("","muted");feedback_->setContentsMargins(20,6,20,6);feedback_->hide();root->addWidget(feedback_);
 auto footer=new QWidget;auto f=new QHBoxLayout(footer);f->setContentsMargins(20,12,20,mobile_?16:12);f->setSpacing(12);
 import_=button("导入答题卡","primary");import_->setObjectName("importButton");import_->setIcon(theme::icon("plus",Qt::white));import_->setIconSize({20,20});save_=button("保存 / 导出");save_->setIcon(theme::icon("export"));save_->setIconSize({20,20});
 if(!mobile_){f->addWidget(label("图像仅在本机处理","muted"));f->addStretch();import_->setMinimumWidth(172);save_->setMinimumWidth(156);}
 f->addWidget(import_,mobile_?1:0);f->addWidget(save_,mobile_?1:0);root->addWidget(divider());root->addWidget(footer);
 connect(import_,&QPushButton::clicked,this,[this]{chooseImage();});connect(save_,&QPushButton::clicked,this,[this]{showExport();});connect(more_,&QPushButton::clicked,this,[this]{showMore();});
 connect(watcher_,&QFutureWatcher<markscan::Result>::finished,this,[this]{result_=watcher_->result();busy_=false;presentResult();});
 QString external=QCoreApplication::applicationDirPath()+"/config/answers.json";if(!loadConfig(QFile::exists(external)?external:":/config/answers.json"))setFailure(error_);else setState(State::Empty);
 if(mobile_)resize(392,820);else{setMinimumSize(920,640);resize(1260,860);}
}
MainWindow::~MainWindow(){if(watcher_->isRunning())watcher_->waitForFinished();}
QWidget*MainWindow::buildEmpty(){
 auto page=new QWidget;auto outer=new QVBoxLayout(page);outer->setContentsMargins(24,20,24,20);auto body=new QWidget;body->setMaximumWidth(760);auto v=new QVBoxLayout(body);v->setContentsMargins(0,0,0,0);v->setSpacing(12);
 v->addWidget(label("从一张答题卡开始","muted"));v->addWidget(label("清晰识别，\n从容判分。","hero"));v->addWidget(label("导入完整答题卡，自动校正与评分。\n每一道题，都能回看识别依据。"));
 auto art=new QLabel;art->setPixmap(illustration().scaled(mobile_?270:340,mobile_?207:261,Qt::KeepAspectRatio,Qt::SmoothTransformation));art->setAlignment(Qt::AlignCenter);v->addWidget(art,1);auto info=label("A4 固定模板  ·  10 道单选  ·  满分 100","muted");info->setAlignment(Qt::AlignCenter);v->addWidget(info);
 auto sample=button("体验样例","quiet");connect(sample,&QPushButton::clicked,this,[this]{showSamples();});v->addWidget(sample);outer->addStretch();outer->addWidget(body,0,Qt::AlignHCenter);outer->addStretch();return page;
}
QWidget*MainWindow::buildBusy(){auto page=new QWidget;auto v=new QVBoxLayout(page);v->setContentsMargins(28,24,28,24);v->addStretch();auto title=label("正在校正与识别","heading");title->setAlignment(Qt::AlignCenter);v->addWidget(title);auto sub=label("检查定位标记与填涂区域，请稍候。","muted");sub->setAlignment(Qt::AlignCenter);v->addWidget(sub);auto bar=new QProgressBar;bar->setRange(0,0);bar->setTextVisible(false);bar->setMaximumWidth(230);v->addSpacing(20);v->addWidget(bar,0,Qt::AlignHCenter);v->addStretch();return page;}
QWidget*MainWindow::buildFailure(){auto page=new QWidget;auto v=new QVBoxLayout(page);v->setContentsMargins(28,24,28,24);v->setSpacing(16);v->addStretch();v->addWidget(label("需要重新选取图片","muted"));v->addWidget(label("未能识别这张答题卡","heading"));failureText_=label("");v->addWidget(failureText_);auto help=button("查看模板要求");v->addWidget(help);connect(help,&QPushButton::clicked,this,[this]{showTemplateHelp();});auto original=button("查看原图","quiet");v->addWidget(original);connect(original,&QPushButton::clicked,this,[this]{if(!original_.isNull()){viewIndex_=2;showPreview();}else notify("当前没有可预览的图片");});v->addStretch();return page;}
QWidget*MainWindow::buildPreview(){
 auto panel=new QWidget;auto v=new QVBoxLayout(panel);v->setContentsMargins(0,0,0,0);v->setSpacing(12);auto h=new QHBoxLayout;h->addWidget(label("答题卡","section"));h->addStretch();auto expand=button("放大查看","quiet");expand->setIcon(theme::icon("expand",QColor(theme::Accent)));h->addWidget(expand);connect(expand,&QPushButton::clicked,this,[this]{showPreview();});v->addLayout(h);
 auto seg=new QFrame;seg->setObjectName("segments");auto s=new QHBoxLayout(seg);s->setContentsMargins(4,4,4,4);s->setSpacing(2);auto group=new QButtonGroup(this);const QStringList names={"标注结果","校正图像","原始图片"};
 for(int i=0;i<3;++i){segments_[i]=button(names[i],"segment");segments_[i]->setCheckable(true);group->addButton(segments_[i],i);s->addWidget(segments_[i],1);connect(segments_[i],&QPushButton::clicked,this,[this,i]{selectView(i);});}segments_[0]->setChecked(true);v->addWidget(seg);
 canvas_=new ImageCanvas(!mobile_);if(mobile_)canvas_->setFixedHeight(272);else canvas_->setMinimumHeight(340);canvas_->activated=[this]{showPreview();};v->addWidget(canvas_,1);
 if(!mobile_){auto c=new QHBoxLayout;auto fit=button("适应窗口","quiet"),actual=button("100%","quiet");c->addWidget(fit);c->addWidget(actual);c->addStretch();c->addWidget(label("滚轮缩放 · 拖动平移","muted"));v->addLayout(c);connect(fit,&QPushButton::clicked,canvas_,[this]{canvas_->fit();});connect(actual,&QPushButton::clicked,canvas_,[this]{canvas_->actualSize();});}return panel;
}
QWidget*MainWindow::buildResults(){
 auto page=new QWidget;auto base=new QHBoxLayout(page);base->setContentsMargins(mobile_?0:24,mobile_?0:18,mobile_?0:24,0);auto content=new QWidget;auto v=new QVBoxLayout(content);v->setSizeConstraint(QLayout::SetMinAndMaxSize);v->setContentsMargins(mobile_?20:8,16,mobile_?20:8,24);v->setSpacing(16);
 source_=label("","muted");v->addWidget(source_);score_=new ScoreCard;v->addWidget(score_);auto stats=new QHBoxLayout;stats->setSpacing(6);const QStringList names={"正确","错误","漏选","多选"};const char*colors[]={theme::Correct,theme::Wrong,theme::Unanswered,theme::Multiple};
 for(int i=0;i<4;++i){auto col=new QVBoxLayout;col->setSpacing(3);stats_[i]=label("0");stats_[i]->setAlignment(Qt::AlignCenter);stats_[i]->setStyleSheet(QString("font-size:23px;font-weight:600;color:%1;").arg(colors[i]));col->addWidget(stats_[i]);auto l=label(names[i],"muted");l->setAlignment(Qt::AlignCenter);col->addWidget(l);stats->addLayout(col,1);}v->addLayout(stats);reviewHint_=label("","muted");v->addWidget(reviewHint_);
 if(mobile_)v->addWidget(buildPreview());v->addWidget(divider());auto qh=new QHBoxLayout;qh->addWidget(label("逐题结果","section"));qh->addStretch();filter_=button("仅待复核","quiet");filter_->setCheckable(true);qh->addWidget(filter_);v->addLayout(qh);connect(filter_,&QPushButton::toggled,this,[this](bool c){onlyReview_=c;renderQuestions();});
 auto questions=new QWidget;questionLayout_=new QVBoxLayout(questions);questionLayout_->setSizeConstraint(QLayout::SetMinAndMaxSize);questionLayout_->setContentsMargins(0,0,0,0);questionLayout_->setSpacing(0);v->addWidget(questions);v->addStretch();resultsScroll_=new QScrollArea;resultsScroll_->setWidgetResizable(true);resultsScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);resultsScroll_->setWidget(content);
 if(mobile_){QScroller::grabGesture(resultsScroll_->viewport(),QScroller::TouchGesture);base->addWidget(resultsScroll_);}else{auto splitter=new QSplitter;splitter->setChildrenCollapsible(false);auto preview=buildPreview();preview->setMinimumWidth(400);resultsScroll_->setMinimumWidth(330);splitter->addWidget(preview);splitter->addWidget(resultsScroll_);splitter->setStretchFactor(0,3);splitter->setStretchFactor(1,2);splitter->setSizes({710,420});base->addWidget(splitter);}return page;
}
void MainWindow::setState(State s){state_=s;screens_->setCurrentIndex(s==State::Empty?0:s==State::Busy?1:s==State::Ready?2:3);if(import_)updateActions();}
void MainWindow::updateActions(){import_->setEnabled(!busy_);more_->setEnabled(!busy_);save_->setEnabled(!busy_&&state_==State::Ready);import_->setText(busy_?"正在识别…":state_==State::Ready?"扫描下一张":"导入答题卡");}
void MainWindow::notify(const QString&m){feedback_->setText(m);feedback_->show();QTimer::singleShot(6500,this,[this]{feedback_->hide();});}
void MainWindow::chooseImage(){auto p=QFileDialog::getOpenFileName(this,"选择答题卡图片",{},"Images (*.png *.jpg *.jpeg *.bmp);;All files (*)");if(!p.isEmpty())startImport(p);}
bool MainWindow::loadConfig(const QString&p){
 QString e;auto bytes=storage::read(p,e);if(bytes.isEmpty()){error_="无法读取配置文件："+e;return false;}QJsonParseError parse;auto doc=QJsonDocument::fromJson(bytes,&parse);auto obj=doc.object();auto answers=obj["answers"].toArray();
 if(parse.error!=QJsonParseError::NoError||obj["template"].toString()!="MarkScan-v1"||answers.size()!=10||obj["points_per_question"].toInt()!=10||!obj["fill_threshold"].isDouble()||obj["fill_threshold"].toDouble()<=0||obj["fill_threshold"].toDouble()>=1){error_="配置需包含10个A–D答案、每题10分与有效阈值。";return false;}markscan::Config candidate;candidate.fillThreshold=obj["fill_threshold"].toDouble();
 for(int i=0;i<10;++i){auto a=answers[i].toString();if(a.size()!=1||a[0]<'A'||a[0]>'D'){error_="标准答案必须为A、B、C或D。";return false;}candidate.answers[i]=a[0].unicode()-'A';}config_=candidate;configPath_=p;configured_=true;return true;
}
void MainWindow::setFailure(const QString&m){result_={};busy_=false;error_=m;failureText_->setText(m);setState(State::Failure);renderQuestions();}
bool MainWindow::decodeImage(const QString&p,cv::Mat&bgr){
 qInfo().noquote()<<"MARKSCAN_IMPORT"<<p;path_=p;original_={};result_={};feedback_->hide();if(!configured_){setFailure("尚未加载有效的标准答案配置。");return false;}
 QString e;auto bytes=storage::read(p,e);if(bytes.isEmpty()){setFailure("图片读取失败："+e);return false;}QBuffer buffer(&bytes);buffer.open(QIODevice::ReadOnly);QImageReader reader(&buffer);reader.setAutoTransform(true);original_=reader.read();if(original_.isNull()){setFailure("图片读取失败："+reader.errorString());return false;}auto rgb=original_.convertToFormat(QImage::Format_RGB888);cv::cvtColor(cv::Mat(rgb.height(),rgb.width(),CV_8UC3,rgb.bits(),rgb.bytesPerLine()),bgr,cv::COLOR_RGB2BGR);return true;
}
bool MainWindow::importImage(const QString&p){if(busy_)return false;cv::Mat bgr;if(!decodeImage(p,bgr))return false;result_=markscan::recognize(bgr,config_);presentResult();return result_.success;}
void MainWindow::startImport(const QString&p){if(busy_)return;busy_=true;setState(State::Busy);QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);cv::Mat bgr;if(!decodeImage(p,bgr))return;auto cfg=config_;watcher_->setFuture(QtConcurrent::run([bgr,cfg]{return markscan::recognize(bgr,cfg);}));}
void MainWindow::presentResult(){
 if(!result_.success){setFailure("未检测到完整的 MarkScan 答题卡。\n\n请保留四角黑色标记，拍清整张卡，并使用本项目固定模板。");return;}error_.clear();int counts[4]={};for(auto&q:result_.questions){using markscan::Status;counts[q.status==Status::Correct?0:q.status==Status::Wrong?1:q.status==Status::Unanswered?2:3]++;}
 const QStringList names={"正确","错误","漏选","多选"};for(int i=0;i<4;++i){stats_[i]->setText(QString::number(counts[i]));stats_[i]->setAccessibleName(names[i]+" "+QString::number(counts[i]));}score_->setResult(result_);
 source_->setText(path_.startsWith("content://")?"识别完成 · 来自系统选图器":"识别完成 · "+QFileInfo(path_).fileName().left(45));reviewHint_->setText(counts[0]==10?"全部答对。可查看标注，或保存本次结果。":QString("%1 道题待复核，点击题目可查看填涂比例。").arg(10-counts[0]));onlyReview_=false;filter_->setChecked(false);viewIndex_=0;segments_[0]->setChecked(true);renderQuestions();updatePreview();setState(State::Ready);resultsScroll_->verticalScrollBar()->setValue(0);
 QStringList selected,statuses;for(auto&q:result_.questions){selected<<(q.selected.empty()?QString("-"):QString::fromStdString(q.selected));statuses<<markscan::statusName(q.status);}qInfo().noquote()<<"MARKSCAN_RESULT version=1.1.0 score="<<result_.score<<" selected="<<selected.join(',')<<" status="<<statuses.join(',');
}
void MainWindow::renderQuestions(){
 while(auto item=questionLayout_->takeAt(0)){if(item->widget())delete item->widget();delete item;}shownQuestions_=0;if(!result_.success)return;
 for(int q=0;q<10;++q){auto item=result_.questions[q];if(onlyReview_&&item.status==markscan::Status::Correct)continue;++shownQuestions_;auto block=new QWidget;auto b=new QVBoxLayout(block);b->setContentsMargins(0,0,0,0);b->setSpacing(0);auto row=button("","question");
 row->setFixedHeight(theme::QuestionHeight);row->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);auto h=new QHBoxLayout(row);h->setContentsMargins(5,8,5,8);h->setSpacing(8);auto num=label(QString::number(q+1).rightJustified(2,'0'),"muted");num->setFixedWidth(25);h->addWidget(num);auto answers=new QVBoxLayout;answers->setSpacing(2);QString selected=item.selected.empty()?"未填":QString::fromStdString(item.selected);auto sl=label(selected,"answer");answers->addWidget(sl);answers->addWidget(label(QString("答案 %1").arg(QChar('A'+config_.answers[q])),"muted"));h->addLayout(answers,1);
 auto status=label(statusText(item.status));status->setStyleSheet(QString("color:%1;font-size:13px;font-weight:600;").arg(statusColor(item.status).name()));h->addWidget(status);auto pts=label(QString::number(item.points)+" 分","muted");pts->setMinimumWidth(30);pts->setAlignment(Qt::AlignRight|Qt::AlignVCenter);h->addWidget(pts);for(auto child:row->findChildren<QLabel*>())child->setAttribute(Qt::WA_TransparentForMouseEvents);row->setAccessibleName(QString("第%1题 识别%2 标准%3 %4 %5分").arg(q+1).arg(selected).arg(QChar('A'+config_.answers[q])).arg(statusText(item.status)).arg(item.points));b->addWidget(row);
 auto ratios=label(QString("填涂比例  A %1%   B %2%   C %3%   D %4%").arg(qRound(item.ratios[0]*100)).arg(qRound(item.ratios[1]*100)).arg(qRound(item.ratios[2]*100)).arg(qRound(item.ratios[3]*100)),"muted");ratios->setContentsMargins(8,10,8,12);ratios->hide();b->addWidget(ratios);connect(row,&QPushButton::clicked,ratios,[ratios]{ratios->setVisible(ratios->isHidden());});questionLayout_->addWidget(block);}
 if(!shownQuestions_)questionLayout_->addWidget(label("没有待复核题目。","muted"));
}
QImage MainWindow::currentImage()const{return viewIndex_==2?original_:toImage(viewIndex_==1?result_.corrected:result_.annotated);}
void MainWindow::updatePreview(){canvas_->setImage(currentImage());}void MainWindow::selectView(int i){viewIndex_=i;updatePreview();}
void MainWindow::showMore(){QMenu menu(this);auto samples=menu.addAction("体验样例"),config=menu.addAction("标准答案"),help=menu.addAction("模板与填写要求");menu.addSeparator();auto about=menu.addAction("关于 MarkScan");auto a=menu.exec(more_->mapToGlobal(QPoint(more_->width(),more_->height())));if(a==samples)showSamples();else if(a==config){auto p=QFileDialog::getOpenFileName(this,"选择标准答案配置",{},"JSON (*.json)");if(!p.isEmpty()){if(!loadConfig(p))notify(error_);else{notify("标准答案已更新");if(!path_.isEmpty())startImport(path_);}}}else if(a==help)showTemplateHelp();else if(a==about)QMessageBox::information(this,"关于 MarkScan","MarkScan 1.1.0\n基于 Qt 与 OpenCV 的答题卡评分工具\n\n本地处理 · 固定模板 · 10题单选\n现有80/80仅代表合成样例，未测纸质照片准确率。");}
void MainWindow::showSamples(){QDialog dialog(this);dialog.setWindowTitle("体验样例");auto v=new QVBoxLayout(&dialog);v->setContentsMargins(20,20,20,20);v->setSpacing(8);v->addWidget(label("用样例体验完整流程","heading"));v->addWidget(label("样例同样逐像素识别，不按文件名评分。","muted"));const QStringList labels={"全部答对","部分答错","漏选样例","多选样例","透视倾斜","无答题卡"},names={"all_correct","partial_wrong","unanswered","multiple","perspective","no_card"};for(int i=0;i<labels.size();++i){auto b=button(labels[i]);v->addWidget(b);connect(b,&QPushButton::clicked,&dialog,[this,&dialog,n=names[i]]{dialog.accept();QTimer::singleShot(0,this,[this,n]{startImport(":/tests/images/"+n+".png");});});}auto close=button("取消","quiet");v->addWidget(close);connect(close,&QPushButton::clicked,&dialog,&QDialog::reject);dialog.setMinimumWidth(mobile_?std::min(width()-32,360):360);dialog.exec();}
void MainWindow::showTemplateHelp(){QMessageBox::information(this,"模板与填写要求","使用 MarkScan A4纵向固定模板。\n\n• 10道单选题，每题10分。\n• 每题只填一个选项，充分涂黑。\n• 四角标记与顶部方向码须完整。\n• 避免反光、遮挡与严重透视。\n\n打印模板随交付包提供。可先用系统相机拍摄，再从本应用导入照片。");}
void MainWindow::showExport(){if(!result_.success)return;QMenu menu(this);auto all=menu.addAction("保存完整结果");menu.addSeparator();auto a=menu.addAction("导出标注 PNG"),c=menu.addAction("导出校正 PNG"),o=menu.addAction("导出原图 PNG");auto chosen=menu.exec(save_->mapToGlobal(QPoint(0,-menu.sizeHint().height())));if(chosen==all)saveAll();else if(chosen==a)exportPng(0);else if(chosen==c)exportPng(1);else if(chosen==o)exportPng(2);}
void MainWindow::saveAll(){
#ifdef Q_OS_ANDROID
 auto dir=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/results/"+QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
#else
 auto dir=QFileDialog::getExistingDirectory(this,"选择结果保存目录");if(dir.isEmpty())return;dir+="/MarkScan_"+QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
#endif
 if(saveResults(dir))QMessageBox::information(this,"结果已保存","JSON、校正图与标注图已保存到：\n"+dir+"\n\n也可从导出菜单将PNG保存到公共位置。");else notify("保存失败，请检查目录是否可写。");
}
void MainWindow::exportPng(int i){auto img=i==2?original_:toImage(i==1?result_.corrected:result_.annotated);if(img.isNull())return;auto p=QFileDialog::getSaveFileName(this,"导出图片","MarkScan-1.1-result.png","PNG (*.png)");if(!p.isEmpty())notify(storage::savePng(p,img)?"图片已导出":"导出失败，无法写入所选文件。");}
void MainWindow::showPreview(){
 if(currentImage().isNull())return;QDialog dialog(this);previewDialog_=&dialog;dialog.setWindowTitle("图像预览");auto v=new QVBoxLayout(&dialog);v->setContentsMargins(16,12,16,16);v->setSpacing(10);auto header=new QHBoxLayout;header->addWidget(label("图像预览","heading"));header->addStretch();auto close=button("关闭","quiet");header->addWidget(close);v->addLayout(header);connect(close,&QPushButton::clicked,&dialog,&QDialog::reject);
 auto choices=new QHBoxLayout;auto group=new QButtonGroup(&dialog);const QStringList names={"标注结果","校正图像","原始图片"};fullCanvas_=new ImageCanvas(true);for(int i=0;i<3;++i){auto b=button(names[i],"segment");b->setCheckable(true);b->setChecked(i==viewIndex_);group->addButton(b);choices->addWidget(b,1);connect(b,&QPushButton::clicked,&dialog,[this,i]{fullCanvas_->setImage(i==2?original_:toImage(i==1?result_.corrected:result_.annotated));});}v->addLayout(choices);v->addWidget(fullCanvas_,1);fullCanvas_->setImage(currentImage());
 auto tools=new QHBoxLayout;auto fit=button("适应","quiet"),actual=button("100%","quiet"),minus=button("−"),plus=button("+");minus->setAccessibleName("缩小");plus->setAccessibleName("放大");minus->setFixedWidth(48);plus->setFixedWidth(48);tools->addWidget(fit);tools->addWidget(actual);tools->addStretch();tools->addWidget(minus);tools->addWidget(plus);v->addLayout(tools);v->addWidget(label(mobile_?"单指平移 · 双指缩放 · 按钮精细调整":"滚轮缩放 · 鼠标拖动 · 适应窗口","muted"));connect(fit,&QPushButton::clicked,fullCanvas_,[this]{fullCanvas_->fit();});connect(actual,&QPushButton::clicked,fullCanvas_,[this]{fullCanvas_->actualSize();});connect(minus,&QPushButton::clicked,fullCanvas_,[this]{fullCanvas_->zoomBy(0.8);});connect(plus,&QPushButton::clicked,fullCanvas_,[this]{fullCanvas_->zoomBy(1.25);});if(mobile_)dialog.resize(size());else dialog.resize(1000,850);dialog.exec();previewDialog_=nullptr;fullCanvas_=nullptr;
}
QJsonObject MainWindow::resultJson()const{QJsonObject o{{"application","MarkScan"},{"version","1.1.0"},{"template","MarkScan-v1"},{"success",result_.success},{"source",path_},{"configuration",configPath_},{"timestamp",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},{"fill_threshold",config_.fillThreshold}};if(!result_.success){o["error"]=error_;return o;}o["score"]=result_.score;o["maximum"]=100;QJsonArray questions;for(int q=0;q<10;++q){const auto&i=result_.questions[q];QJsonArray ratios;for(auto r:i.ratios)ratios.append(r);questions.append(QJsonObject{{"question",q+1},{"selected",QString::fromStdString(i.selected)},{"expected",QString(QChar('A'+config_.answers[q]))},{"status",markscan::statusName(i.status)},{"points",i.points},{"fill_ratios_ABCD",ratios}});}o["questions"]=questions;return o;}
bool MainWindow::saveResults(const QString&d){return result_.success&&QDir().mkpath(d)&&toImage(result_.corrected).save(d+"/corrected.png")&&toImage(result_.annotated).save(d+"/annotated.png")&&writeJson(d+"/result.json",resultJson());}
void MainWindow::runSmoke(const QString&root){
 QString base=root+"/docs/upgrade";QDir().mkpath(base+"/evidence");QDir().mkpath(base+"/logs");QString prefix=mobile_?"mobile-layout":"windows";grab().save(base+"/evidence/"+prefix+"-empty.png");QJsonArray cases;bool all=true;const QStringList names={"all_correct","partial_wrong","unanswered","multiple","perspective","no_card"};const int scores[]={100,70,80,90,100,0};
 for(int i=0;i<names.size();++i){bool ok=importImage(root+"/tests/images/"+names[i]+".png");bool pass=ok==(i!=5);if(ok)pass=pass&&result_.score==scores[i]&&shownQuestions_==10&&saveResults(base+"/evidence/"+prefix+"_"+names[i]);pass=pass&&save_->isEnabled()==ok;QApplication::processEvents();grab().save(base+"/evidence/"+prefix+"-"+names[i]+".png");cases.append(QJsonObject{{"case",names[i]},{"passed",pass},{"actual",resultJson()}});all&=pass;}
 importImage(root+"/tests/images/multiple.png");filter_->setChecked(true);bool filterPass=shownQuestions_==1;all&=filterPass;QEventLoop loop;connect(watcher_,&QFutureWatcher<markscan::Result>::finished,&loop,&QEventLoop::quit);QTimer timer;timer.setSingleShot(true);connect(&timer,&QTimer::timeout,&loop,&QEventLoop::quit);timer.start(30000);startImport(root+"/tests/images/perspective.png");grab().save(base+"/evidence/"+prefix+"-busy.png");loop.exec();bool asyncPass=state_==State::Ready&&result_.score==100&&!busy_;all&=asyncPass;
 bool previewPass=false;QTimer::singleShot(250,this,[this,&previewPass,base,prefix]{if(fullCanvas_&&previewDialog_){double before=fullCanvas_->scale();fullCanvas_->zoomBy(2);previewPass=fullCanvas_->scale()>before;previewDialog_->grab().save(base+"/evidence/"+prefix+"-preview.png");previewDialog_->reject();}});showPreview();all&=previewPass;bool corruptPass=!importImage(root+"/tests/images/nonexistent.png")&&!save_->isEnabled();all&=corruptPass;
 writeJson(base+"/logs/"+prefix+"-ui-tests.json",QJsonObject{{"version","1.1.0"},{"passed",all},{"cases",cases},{"filter",filterPass},{"async_import",asyncPass},{"preview_zoom",previewPass},{"invalid_file",corruptPass}});qInfo()<<"MARKSCAN_UI_SMOKE"<<(all?"PASS":"FAIL");QApplication::exit(all?0:1);
}
void MainWindow::runDemo(){const QStringList names={"all_correct","partial_wrong","unanswered","multiple","perspective","no_card"};for(int i=0;i<names.size();++i)QTimer::singleShot(2000+i*4500,this,[this,n=names[i]]{startImport(":/tests/images/"+n+".png");});}
