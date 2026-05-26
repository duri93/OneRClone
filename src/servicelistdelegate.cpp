#include "servicelistdelegate.h"
#include "servicelistmodel.h"
#include "mountservice.h"

#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include <QStyleOptionButton>
#include <QAbstractItemView>

static constexpr int ROW_HEIGHT   = 36;
static constexpr int ICON_SIZE    = 14;
static constexpr int BTN_WIDTH    = 54;
static constexpr int BTN_HEIGHT   = 24;
static constexpr int H_PADDING    = 6;

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------
QRect ServiceListDelegate::iconRect(const QRect& r) const
{
    int y = r.top() + (r.height() - ICON_SIZE) / 2;
    return QRect(r.left() + H_PADDING, y, ICON_SIZE, ICON_SIZE);
}

QRect ServiceListDelegate::buttonRect(const QRect& r) const
{
    int x = r.right() - BTN_WIDTH - H_PADDING;
    int y = r.top()   + (r.height() - BTN_HEIGHT) / 2;
    return QRect(x, y, BTN_WIDTH, BTN_HEIGHT);
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
ServiceListDelegate::ServiceListDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{}

// ---------------------------------------------------------------------------
// sizeHint
// ---------------------------------------------------------------------------
QSize ServiceListDelegate::sizeHint(const QStyleOptionViewItem&,
                                    const QModelIndex&) const
{
    return QSize(0, ROW_HEIGHT);
}

// ---------------------------------------------------------------------------
// eventFilter
// ---------------------------------------------------------------------------
bool ServiceListDelegate::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseMove) {
        auto* vp = qobject_cast<QWidget*>(obj);
        auto* view = qobject_cast<QAbstractItemView*>(vp->parent());
        if (view) {
            auto* me = static_cast<QMouseEvent*>(event);
            QModelIndex index = view->indexAt(me->pos());
            if (!index.isValid() && m_hoveredRow >= 0) {
                // Mouse is in empty space — clear the previous row
                int prevRow  = m_hoveredRow;
                m_hoveredRow    = -1;
                m_buttonHovered = false;
                QModelIndex prevIndex = view->model()->index(prevRow, 0);
                view->model()->dataChanged(prevIndex, prevIndex);
            }
        }
    }

    if (event->type() == QEvent::Leave) {
        m_hoveredRow    = -1;
        m_buttonHovered = false;
        // Trigger a full repaint of the viewport
        if (auto* vp = qobject_cast<QWidget*>(obj))
            vp->update();
    }
    return QStyledItemDelegate::eventFilter(obj, event);
}

// ---------------------------------------------------------------------------
// paint
// ---------------------------------------------------------------------------
void ServiceListDelegate::paint(QPainter* painter,
                                 const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    painter->save();

    // Background (selection / hover)
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    if (index.row() == m_hoveredRow) {
        painter->fillRect(option.rect,
                          option.palette.color(QPalette::Base).lighter(120));
    }

    if (index.row() == m_hoveredRow) opt.state |= QStyle::State_MouseOver | QStyle::State_Active;
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter);

    const auto status = static_cast<ServiceStatus>(
        index.data(ServiceRole::Status).toInt());
    const QString name = index.data(ServiceRole::Name).toString();

    // ---- Status icon (filled circle) ----
    QRect ir = iconRect(option.rect);
    QColor dot;
    switch (status) {
        case ServiceStatus::Stopped:  dot = QColor(0xAA, 0xAA, 0xAA); break;  // grey
        case ServiceStatus::Starting: dot = QColor(0xFF, 0xCC, 0x00); break;  // yellow
        case ServiceStatus::Running:  dot = QColor(0x22, 0xBB, 0x22); break;  // green
        case ServiceStatus::Stopping: dot = QColor(0xFF, 0xCC, 0x00); break;  // yellow
        case ServiceStatus::Errored:  dot = QColor(0xDD, 0x22, 0x22); break;  // red
    }
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(dot);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(ir);

    // ---- Service name ----
    QRect btnR = buttonRect(option.rect);
    QRect textR = option.rect.adjusted(
        H_PADDING + ICON_SIZE + H_PADDING, 0,
        -(BTN_WIDTH + 2 * H_PADDING),      0);

    painter->setPen(option.palette.color(
        (option.state & QStyle::State_Selected)
            ? QPalette::HighlightedText : QPalette::Text));
    painter->drawText(textR, Qt::AlignVCenter | Qt::AlignLeft, name);

    // ---- Start / Stop button ----
    QStyleOptionButton btnOpt;
    btnOpt.rect  = btnR;
    btnOpt.state = QStyle::State_Enabled;
    if (index.row() == m_hoveredRow && m_buttonHovered) btnOpt.state |= QStyle::State_MouseOver;
    if (index.row() == m_pressedRow && m_buttonPressed) btnOpt.state |= QStyle::State_Sunken;
    btnOpt.text  = (status == ServiceStatus::Running  ||
                    status == ServiceStatus::Starting) ? "Stop" : "Start";

    QApplication::style()->drawControl(QStyle::CE_PushButton, &btnOpt, painter);

    painter->restore();
}

// ---------------------------------------------------------------------------
// editorEvent – detect button click
// ---------------------------------------------------------------------------
bool ServiceListDelegate::editorEvent(QEvent* event,
                                       QAbstractItemModel* model,
                                       const QStyleOptionViewItem& option,
                                       const QModelIndex& index)
{
    const bool overButton = buttonRect(option.rect).contains(
        static_cast<QMouseEvent*>(event)->pos());

    switch (event->type()) {

    case QEvent::MouseMove: {
        int  prevRow     = m_hoveredRow;
        bool prevButton  = m_buttonHovered;

        m_hoveredRow    = index.row();
        m_buttonHovered = overButton;

        if (prevRow != m_hoveredRow || prevButton != m_buttonHovered) {
            // Repaint the previously hovered row to clear its highlight
            if (prevRow >= 0 && prevRow != m_hoveredRow) {
                QModelIndex prevIndex = model->index(prevRow, 0);
                const_cast<QAbstractItemModel*>(model)->dataChanged(prevIndex, prevIndex);
            }
            // Repaint the current row
            const_cast<QAbstractItemModel*>(model)->dataChanged(index, index);
        }

        return false;
    }

    /*case QEvent::MouseMove: {
        bool changed = false;
        if (m_hoveredRow != index.row()) {
            m_hoveredRow    = index.row();
            m_buttonHovered = overButton;
            changed = true;
        } else if (m_buttonHovered != overButton) {
            m_buttonHovered = overButton;
            changed = true;
        }
        if (changed)
            const_cast<QAbstractItemModel*>(model)->dataChanged(index, index);
        return false;  // don't consume — let the view handle row highlight too
    }*/

    case QEvent::MouseButtonPress: {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton && overButton) {
            m_pressedRow    = index.row();
            m_buttonPressed = true;
            const_cast<QAbstractItemModel*>(model)->dataChanged(index, index);
            return true;
        }
        return false;
    }

    case QEvent::MouseButtonRelease: {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            bool wasPressed = (m_pressedRow == index.row() && m_buttonPressed);
            m_pressedRow    = -1;
            m_buttonPressed = false;
            const_cast<QAbstractItemModel*>(model)->dataChanged(index, index);
            if (wasPressed && overButton)
                emit toggleRequested(index);
            return wasPressed;
        }
        return false;
    }

    default:
        return false;
    }
}
