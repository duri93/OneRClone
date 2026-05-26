#include "servicelistmodel.h"

ServiceListModel::ServiceListModel(QObject* parent)
    : QAbstractListModel(parent)
{}

int ServiceListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_services.size());
}

QVariant ServiceListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= static_cast<int>(m_services.size())) {
        return {};
    }

    const MountService* svc = m_services.at(index.row()).get();

    switch (role) {
        case Qt::DisplayRole:
        case ServiceRole::Name:
            return svc->name();
        case ServiceRole::Id:
            return svc->id();
        case ServiceRole::Status:
            return static_cast<int>(svc->status());
        default:
            return {};
    }
}

// ---------------------------------------------------------------------------
// Service management
// ---------------------------------------------------------------------------
void ServiceListModel::addService(const ServiceSpec& spec, const SharedSettings& shared)
{
    beginInsertRows({}, static_cast<int>(m_services.size()),
                        static_cast<int>(m_services.size()));

    auto svc = std::make_unique<MountService>(spec, shared, this);

    connect(svc.get(), &MountService::statusChanged,
            this, &ServiceListModel::onStatusChanged);
    connect(svc.get(), &MountService::outputLine,
            this, &ServiceListModel::serviceOutputLine);

    m_services.push_back(std::move(svc));
    endInsertRows();
}

void ServiceListModel::removeService(const QString& id)
{
    for (int i = 0; i < static_cast<int>(m_services.size()); ++i) {
        if (m_services[i]->id() == id) {
            m_services[i]->stop();
            beginRemoveRows({}, i, i);
            m_services.erase(m_services.begin() + i);
            endRemoveRows();
            return;
        }
    }
}

void ServiceListModel::updateSpec(const QString& id, const ServiceSpec& spec)
{
    int row = rowForId(id);
    if (row < 0) return;
    m_services[row]->updateSpec(spec);
    emit dataChanged(index(row), index(row));
}

void ServiceListModel::updateAllShared(const SharedSettings& shared)
{
    for (auto& svc : m_services) {
        svc->updateShared(shared);
    }
}

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------
MountService* ServiceListModel::serviceById(const QString& id) const
{
    for (const auto& svc : m_services) {
        if (svc->id() == id) return svc.get();
    }
    return nullptr;
}

MountService* ServiceListModel::serviceAtRow(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_services.size())) return nullptr;
    return m_services[row].get();
}

int ServiceListModel::rowForId(const QString& id) const
{
    for (int i = 0; i < static_cast<int>(m_services.size()); ++i) {
        if (m_services[i]->id() == id) return i;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------
void ServiceListModel::onStatusChanged(const QString& id, ServiceStatus status)
{
    int row = rowForId(id);
    if (row >= 0) {
        emit dataChanged(index(row), index(row), {ServiceRole::Status});
    }
    emit serviceStatusChanged(id, status);
}
