#pragma once
#include <QDialog>
#include <QString>

// Frameless themed dialog matching Python YDMDialog
class YDMDialog : public QDialog {
    Q_OBJECT
public:
    enum class Kind { Error, Warning, Info, Question };

    explicit YDMDialog(QWidget*       parent,
                       Kind           kind,
                       const QString& title,
                       const QString& message);

    bool resultYes() const { return m_accepted; }

    // Static convenience methods
    static void  showError   (QWidget* parent, const QString& title, const QString& msg);
    static void  showWarning (QWidget* parent, const QString& title, const QString& msg);
    static void  showInfo    (QWidget* parent, const QString& title, const QString& msg);
    static bool  showQuestion(QWidget* parent, const QString& title, const QString& msg);

protected:
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev)  override;

private slots:
    void onOk();
    void onCancel();

private:
    bool    m_accepted = false;
    QPoint  m_dragPos;
};
