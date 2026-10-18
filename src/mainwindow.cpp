#include "mainwindow.h"

#include <QtConcurrent/QtConcurrent>

#include <QCheckBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMimeData>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSlider>
#include <QTextEdit>
#include <QFutureWatcher>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

static QString formatLog(const std::vector<JobResult>& results) {
  QStringList lines;
  for (const auto& r : results) {
    const auto in = QString::fromStdWString(r.input.wstring());
    if (r.message != "完成") {
      lines << QString("%1 → %2").arg(in, QString::fromStdString(r.message));
      continue;
    }
    const qint64 saved = static_cast<qint64>(r.bytes_before - r.bytes_after);
    lines << QString("%1 → %2 (%3 KB 节省)")
                 .arg(in, QString::fromStdWString(r.output.wstring()))
                 .arg(saved / 1024);
  }
  return lines.join('\n');
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(QStringLiteral("轻图"));
  resize(520, 720);
  setAcceptDrops(true);

  const QString dark = QStringLiteral(
      "QMainWindow { background: #1A2629; }"
      "QLabel { color: #D2DADE; }"
      "QListWidget { background: #26363C; color: #D2DADE; border: none; }"
      "QLineEdit, QTextEdit { background: #26363C; color: #D2DADE; border: 1px solid #3A4C54; }"
      "QPushButton { background: #1E2C30; color: #828F96; border: 1px solid #3A4C54; padding: 6px 12px; }"
      "QRadioButton, QCheckBox { color: #D2DADE; }"
      "QSlider::groove:horizontal { background: #26363C; height: 6px; }"
      "QSlider::handle:horizontal { background: #48C4B6; width: 14px; margin: -4px 0; }");
  setStyleSheet(dark);

  auto* scroll = new QScrollArea(this);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  auto* root = new QWidget;
  scroll->setWidget(root);
  setCentralWidget(scroll);

  auto* layout = new QVBoxLayout(root);
  layout->setContentsMargins(20, 16, 20, 16);

  auto* title = new QLabel(QStringLiteral("轻图"));
  title->setAlignment(Qt::AlignCenter);
  title->setStyleSheet("font-size: 28px; font-weight: bold; color: #48C4B6;");
  layout->addWidget(title);
  {
    auto* s = new QLabel(QStringLiteral("把体积压下去，细节留住"));
    s->setAlignment(Qt::AlignCenter);
    layout->addWidget(s);
  }
  auto* sub = new QLabel(QStringLiteral("本地处理 · 不上传 · PNG / JPG"));
  sub->setStyleSheet("color: #828F96; font-size: 11px;");
  sub->setAlignment(Qt::AlignCenter);
  layout->addWidget(sub);

  auto* tab_row = new QHBoxLayout;
  btn_tab_compress_ = new QPushButton(QStringLiteral("压缩"));
  btn_tab_stitch_ = new QPushButton(QStringLiteral("拼接"));
  tab_row->addWidget(btn_tab_compress_);
  tab_row->addWidget(btn_tab_stitch_);
  layout->addLayout(tab_row);

  drop_zone_ = new QFrame;
  drop_zone_->setStyleSheet("QFrame { background: #1E2C30; border: 1px solid #48C4B6; border-radius: 6px; }");
  drop_zone_->setAcceptDrops(true);
  auto* drop_layout = new QVBoxLayout(drop_zone_);
  drop_hint_ = new QLabel(QStringLiteral("拖拽图片到这里"));
  drop_hint_->setAlignment(Qt::AlignCenter);
  drop_hint_->setStyleSheet("color: #828F96;");
  drop_layout->addWidget(drop_hint_);
  auto* pick_btn = new QPushButton(QStringLiteral("选择图片"));
  pick_btn->setStyleSheet("background: #E68C46; color: white; font-weight: 600;");
  connect(pick_btn, &QPushButton::clicked, this, &MainWindow::onPickFiles);
  drop_layout->addWidget(pick_btn, 0, Qt::AlignCenter);
  layout->addWidget(drop_zone_);

  file_list_ = new QListWidget;
  file_list_->setFixedHeight(100);
  layout->addWidget(file_list_);

  panel_compress_ = new QWidget;
  auto* pc_layout = new QVBoxLayout(panel_compress_);
  auto* q_row = new QHBoxLayout;
  q_row->addWidget(new QLabel(QStringLiteral("质量")));
  slider_quality_ = new QSlider(Qt::Horizontal);
  slider_quality_->setRange(1, 100);
  slider_quality_->setValue(80);
  q_row->addWidget(slider_quality_, 1);
  txt_quality_ = new QLabel(QStringLiteral("80"));
  q_row->addWidget(txt_quality_);
  pc_layout->addLayout(q_row);
  auto* side_row = new QHBoxLayout;
  side_row->addWidget(new QLabel(QStringLiteral("最长边")));
  txt_max_side_ = new QLineEdit(QStringLiteral("0"));
  txt_max_side_->setFixedWidth(72);
  side_row->addWidget(txt_max_side_);
  side_row->addWidget(new QLabel(QStringLiteral("px（0 = 不缩放）")));
  pc_layout->addLayout(side_row);
  pc_layout->addWidget(new QLabel(QStringLiteral("输出格式")));
  auto* fmt_row = new QHBoxLayout;
  fmt_keep_ = new QRadioButton(QStringLiteral("保持原格式"));
  fmt_jpeg_ = new QRadioButton(QStringLiteral("JPEG"));
  fmt_png_ = new QRadioButton(QStringLiteral("PNG"));
  fmt_webp_ = new QRadioButton(QStringLiteral("WebP"));
  fmt_keep_->setChecked(true);
  fmt_row->addWidget(fmt_keep_);
  fmt_row->addWidget(fmt_jpeg_);
  fmt_row->addWidget(fmt_png_);
  fmt_row->addWidget(fmt_webp_);
  pc_layout->addLayout(fmt_row);
  layout->addWidget(panel_compress_);

  stitch_hint_ = new QLabel(QStringLiteral("按列表顺序横向拼接，高度对齐最短边"));
  stitch_hint_->setStyleSheet("color: #828F96;");
  stitch_hint_->hide();
  layout->addWidget(stitch_hint_);

  chk_custom_out_ = new QCheckBox(QStringLiteral("自定义输出目录"));
  layout->addWidget(chk_custom_out_);
  auto* out_row = new QHBoxLayout;
  txt_out_dir_ = new QLineEdit;
  out_row->addWidget(txt_out_dir_, 1);
  auto* browse = new QPushButton(QStringLiteral("浏览…"));
  connect(browse, &QPushButton::clicked, this, &MainWindow::onPickOutDir);
  out_row->addWidget(browse);
  layout->addLayout(out_row);

  auto* run_row = new QHBoxLayout;
  btn_run_ = new QPushButton;
  btn_run_->setStyleSheet("background: #E68C46; color: white; font-weight: 600;");
  connect(btn_run_, &QPushButton::clicked, this, &MainWindow::onRun);
  run_row->addWidget(btn_run_);
  auto* clear_btn = new QPushButton(QStringLiteral("清空列表"));
  connect(clear_btn, &QPushButton::clicked, this, &MainWindow::onClear);
  run_row->addWidget(clear_btn);
  layout->addLayout(run_row);

  txt_log_ = new QTextEdit;
  txt_log_->setReadOnly(true);
  txt_log_->setMinimumHeight(72);
  txt_log_->setFontFamily(QStringLiteral("Consolas"));
  layout->addWidget(txt_log_);

  connect(btn_tab_compress_, &QPushButton::clicked, this, &MainWindow::onTabCompress);
  connect(btn_tab_stitch_, &QPushButton::clicked, this, &MainWindow::onTabStitch);
  connect(slider_quality_, &QSlider::valueChanged, this, &MainWindow::onQualityChanged);

  updateTabUi();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
  if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event) {
  QStringList paths;
  for (const auto& url : event->mimeData()->urls()) {
    if (url.isLocalFile()) paths << url.toLocalFile();
  }
  addFiles(paths);
  event->acceptProposedAction();
}

void MainWindow::onTabCompress() {
  compress_tab_ = true;
  updateTabUi();
}
void MainWindow::onTabStitch() {
  compress_tab_ = false;
  updateTabUi();
}

void MainWindow::updateTabUi() {
  panel_compress_->setVisible(compress_tab_);
  stitch_hint_->setVisible(!compress_tab_);
  styleTab(btn_tab_compress_, compress_tab_);
  styleTab(btn_tab_stitch_, !compress_tab_);
  updateRunLabel();
}

void MainWindow::styleTab(QPushButton* btn, bool active) {
  if (active)
    btn->setStyleSheet("background: #26363C; color: #48C4B6; font-weight: 600; border: 1px solid #48C4B6;");
  else
    btn->setStyleSheet("background: #1E2C30; color: #828F96; border: 1px solid #3A4C54;");
}

void MainWindow::updateRunLabel() {
  btn_run_->setText(compress_tab_ ? QStringLiteral("开始压缩 (%1)").arg(files_.size())
                                  : QStringLiteral("开始拼接 (%1)").arg(files_.size()));
}

void MainWindow::onQualityChanged(int value) { txt_quality_->setText(QString::number(value)); }

void MainWindow::onPickFiles() {
  const auto paths = QFileDialog::getOpenFileNames(
      this, QStringLiteral("选择图片"), {},
      QStringLiteral("图片 (*.png *.jpg *.jpeg *.webp *.gif *.bmp);;所有文件 (*.*)"));
  addFiles(paths);
}

void MainWindow::onPickOutDir() {
  const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("输出目录"));
  if (dir.isEmpty()) return;
  txt_out_dir_->setText(dir);
  chk_custom_out_->setChecked(true);
}

void MainWindow::addFiles(const QStringList& paths) {
  for (const auto& p : paths) {
    const auto path = std::filesystem::path(p.toStdWString());
    const auto ext = path.extension().wstring();
    static const wchar_t* ok[] = {L".png", L".jpg", L".jpeg", L".webp", L".gif", L".bmp"};
    bool image = false;
    for (auto* e : ok) {
      if (_wcsicmp(ext.c_str(), e) == 0) {
        image = true;
        break;
      }
    }
    if (!image) continue;
    if (std::find(files_.begin(), files_.end(), path) != files_.end()) continue;
    files_.push_back(path);
    file_list_->addItem(QString::fromStdWString(path.filename().wstring()));
  }
  drop_hint_->setText(files_.empty() ? QStringLiteral("拖拽图片到这里")
                                     : QStringLiteral("已选 %1 个文件").arg(files_.size()));
  drop_hint_->setStyleSheet(files_.empty() ? "color: #828F96;" : "color: #48C4B6;");
  updateRunLabel();
}

JobOptions MainWindow::buildOptions() const {
  JobOptions o;
  o.quality = slider_quality_->value();
  o.max_side = txt_max_side_->text().trimmed().toInt();
  if (o.max_side < 0) o.max_side = 0;
  if (fmt_jpeg_->isChecked()) o.format = OutputFormat::Jpeg;
  else if (fmt_png_->isChecked()) o.format = OutputFormat::Png;
  else if (fmt_webp_->isChecked()) o.format = OutputFormat::WebP;
  else o.format = OutputFormat::KeepOriginal;
  o.use_custom_out = chk_custom_out_->isChecked();
  if (o.use_custom_out) o.output_dir = std::filesystem::path(txt_out_dir_->text().trimmed().toStdWString());
  return o;
}

void MainWindow::onRun() {
  if (busy_) return;
  if (files_.empty()) {
    txt_log_->setPlainText(QStringLiteral("请先添加图片"));
    return;
  }
  busy_ = true;
  btn_run_->setEnabled(false);
  const auto opts = buildOptions();
  const bool compress = compress_tab_;
  const auto inputs = files_;

  auto* watcher = new QFutureWatcher<QString>(this);
  connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher]() {
    txt_log_->setPlainText(watcher->result());
    busy_ = false;
    btn_run_->setEnabled(true);
    watcher->deleteLater();
  });
  watcher->setFuture(QtConcurrent::run([compress, inputs, opts]() -> QString {
    if (compress) return formatLog(run_compress_jobs(inputs, opts));
    if (inputs.size() < 2) return QStringLiteral("拼接需要至少 2 张图");
    std::filesystem::path out;
    if (opts.use_custom_out && !opts.output_dir.empty())
      out = opts.output_dir / "stitched.png";
    else if (!inputs.empty()) {
      auto parent = inputs.front().parent_path();
      out = (parent.empty() ? std::filesystem::path("compressed") : parent / "compressed") /
            "stitched.png";
    }
    std::string err;
    if (!stitch_horizontal(inputs, out, err)) return QString::fromStdString(err);
    return QStringLiteral("拼接完成 → %1").arg(QString::fromStdWString(out.wstring()));
  }));
}

void MainWindow::onClear() {
  files_.clear();
  file_list_->clear();
  txt_log_->clear();
  drop_hint_->setText(QStringLiteral("拖拽图片到这里"));
  drop_hint_->setStyleSheet("color: #828F96;");
  updateRunLabel();
}
