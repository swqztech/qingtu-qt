#pragma once

#include "image_job.h"

#include <QMainWindow>
#include <QStringList>
#include <vector>
#include <filesystem>

class QCheckBox;
class QLabel;
class QListWidget;
class QPushButton;
class QRadioButton;
class QSlider;
class QTextEdit;
class QLineEdit;
class QWidget;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);

 protected:
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;

 private slots:
  void onTabCompress();
  void onTabStitch();
  void onPickFiles();
  void onPickOutDir();
  void onRun();
  void onClear();
  void onQualityChanged(int value);

 private:
  void updateTabUi();
  void updateRunLabel();
  void addFiles(const QStringList& paths);
  void styleTab(QPushButton* btn, bool active);
  JobOptions buildOptions() const;

  bool compress_tab_ = true;
  bool busy_ = false;
  std::vector<std::filesystem::path> files_;

  QWidget* drop_zone_ = nullptr;
  QLabel* drop_hint_ = nullptr;
  QListWidget* file_list_ = nullptr;
  QWidget* panel_compress_ = nullptr;
  QLabel* stitch_hint_ = nullptr;
  QPushButton* btn_tab_compress_ = nullptr;
  QPushButton* btn_tab_stitch_ = nullptr;
  QPushButton* btn_run_ = nullptr;
  QSlider* slider_quality_ = nullptr;
  QLabel* txt_quality_ = nullptr;
  QLineEdit* txt_max_side_ = nullptr;
  QRadioButton* fmt_keep_ = nullptr;
  QRadioButton* fmt_jpeg_ = nullptr;
  QRadioButton* fmt_png_ = nullptr;
  QRadioButton* fmt_webp_ = nullptr;
  QCheckBox* chk_custom_out_ = nullptr;
  QLineEdit* txt_out_dir_ = nullptr;
  QTextEdit* txt_log_ = nullptr;
};
