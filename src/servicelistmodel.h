#pragma once

#include "mountservice.h"

#include <QAbstractListModel>
#include <QVector>
#include <memory>

// Custom roles for the service list
namespace ServiceRole {
    inline constexpr int Id     = Qt::UserRole + 1;
    inline constexpr int Name   = Qt::UserRole + 2;
    inline constexpr int Status = Qt::UserRole + 3;   // ServiceStatus (int)
}

// ---------------------------------------------------------------------------
// ServiceListModel
// Owns the MountService instances.  The MainWindow interacts with services
// exclusively through this model and signals.
// ---------------------------------------------------------------------------
class ServiceListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ServiceListModel(QObject* parent = nullptr);

    // QAbstractListModel interface
    int      rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    // Service management
    void addService(const ServiceSpec& spec, const SharedSettings& shared);
    void removeService(const QString& id);
    void updateSpec(const QString& id, const ServiceSpec& spec);
    void updateAllShared(const SharedSettings& shared);

    // Lookup
    MountService* serviceById(const QString& id) const;
    MountService* serviceAtRow(int row) const;
    int           rowForId(const QString& id) const;

signals:
    // Forwarded from individual MountService instances
    void serviceStatusChanged(const QString& id, ServiceStatus status);
    void serviceOutputLine(const QString& id, const QString& line);

private slots:
    void onStatusChanged(const QString& id, ServiceStatus status);

private:
    std::vector<std::unique_ptr<MountService>> m_services;
};
