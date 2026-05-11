#ifndef MPVWIDGET_H
#define MPVWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QString>
#include <mpv/client.h>
#include <mpv/render_gl.h>

class MpvWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit MpvWidget(QWidget *parent = nullptr);
    ~MpvWidget();

    bool loadFile(const QString &fileName);
    void closeFile();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onMpvUpdate();
    void doLoadFile();

private:
    static void on_update(void *ctx);
    static void *get_proc_address(void *ctx, const char *name);
    QString mapQtKeyToMpvKey(QKeyEvent *event);

    mpv_handle *m_mpv;
    mpv_render_context *m_mpvGL;
    QString m_pendingFile;
    bool m_glReady;
};

#endif // MPVWIDGET_H
