#pragma once

#include <QStyledItemDelegate>
#include <QAbstractItemModel>

// ---------------------------------------------------------------------------
// ServiceListDelegate
//
// Draws each row as:
//   [status icon]  [service name]  [Start/Stop button]
//
// The button click is detected in editorEvent and translated into a signal.
// ---------------------------------------------------------------------------
class ServiceListDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ServiceListDelegate(QObject* parent = nullptr);

    QWidget* createEditor(QWidget*, const QStyleOptionViewItem&, const QModelIndex&) const override {
        return nullptr;
    }

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

    bool eventFilter(QObject* obj, QEvent* event) override;

    bool editorEvent(QEvent* event,
                     QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;


signals:
    // Emitted when the user clicks the start/stop button in a row
    void toggleRequested(const QModelIndex& index);
    void repaintNeeded(const QModelIndex&);

private:
    // Returns the bounding rect of the button within a row rect
    QRect buttonRect(const QRect& rowRect) const;
    QRect iconRect(const QRect& rowRect) const;

    mutable int  m_hoveredRow    = -1;
    mutable int  m_pressedRow    = -1;
    mutable bool m_buttonHovered = false;
    mutable bool m_buttonPressed = false;
};
