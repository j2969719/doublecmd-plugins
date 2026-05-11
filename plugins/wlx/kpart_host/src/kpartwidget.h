#ifndef KPARTWIDGET_H
#define KPARTWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <KParts/ReadOnlyPart>
#include <KPluginMetaData>
#include <QUrl>
#include <QPointer>

class KPartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KPartWidget(QWidget *parent = nullptr);
    ~KPartWidget();

    bool loadFile(const QString &fileName);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void installFocusGuard();
    void returnFocusToDC();
    void instantiatePart();

    KParts::ReadOnlyPart *m_part;
    QVBoxLayout *m_layout;
    int m_loadGeneration;
    QUrl m_pendingUrl;
    KPluginMetaData m_selectedPart;
    QPointer<QWidget> m_savedFocusWidget;
};

#endif // KPARTWIDGET_H
