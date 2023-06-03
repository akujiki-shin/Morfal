// Morfal is a Virtual Pen and Paper (VPP): a Game Master's toolbox designed for those who play Role Playing Games with pen and paper only,
// as opposed to Virtual Table Tops (VTT) which are made to handle tactical gameplay (tokens, square/hexagons, area of effect, etc...).
// Copyright (C) 2022  akujiki
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "musicplayer.h"

#include "ui_mainwindow.h"

#include <QAbstractItemModel>
#include <QAudioOutput>
#include <QDirIterator>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QSlider>

#include "searchablemultilistdatawiget.h"
#include "utils.h"

void ExtractMusic(QDirIterator& file, std::vector<std::pair<QString, QString>>& dataList, const QString& extension)
{
    if (file.fileInfo().isDir())
    {
        const QString& path = file.fileInfo().absolutePath() + "/" + file.fileName();
        QDirIterator it(path, QDir::Files);

        bool hasNext = true;
        while(hasNext)
        {
            if (it.fileInfo().isFile() && Utils::FileMatchesExtension(it.fileName(), extension))
            {
                QString data = it.filePath();
                dataList.push_back(std::make_pair(it.fileName(), std::move(data)));
            }

            hasNext = it.hasNext();
            it.next();
        }
    }
}

MusicPlayer::MusicPlayer(Ui::MainWindow* mainWindowUI, QObject *parent)
    : super(parent)
    , ui(mainWindowUI)
{
    SearchableMultiListDataWiget::DirDataExtractor musicExtractor = ExtractMusic;

    ui->musicDataList->ListData("music", ".mp3", musicExtractor, false);

    connect(ui->musicDataList, &SearchableMultiListDataWiget::SectionItemClicked, this, &MusicPlayer::SectionItemClicked);
    connect(ui->musicDataList, &SearchableMultiListDataWiget::SectionItemDoubleClicked, this, &MusicPlayer::SectionItemDoubleClicked);

    connect(ui->playMusicButton, &QPushButton::clicked, this, &MusicPlayer::OnPlayMusicButtonClicked);
    connect(ui->pauseMusicButton, &QPushButton::clicked, this, &MusicPlayer::OnPauseMusicButtonClicked);
    connect(ui->stopMusicButton, &QPushButton::clicked, this, &MusicPlayer::OnStopMusicButtonClicked);
    connect(ui->removeFromPlaylist, &QPushButton::clicked, this, &MusicPlayer::OnRemoveFromPlaylistClicked);
    connect(ui->addToPlaylistButton, &QPushButton::clicked, this, &MusicPlayer::OnAddToPlaylistButtonClicked);
    connect(ui->verticalSlider, &QSlider::valueChanged, this, &MusicPlayer::OnVerticalSliderValueChanged);
    connect(ui->horizontalSlider, &QSlider::sliderReleased, this, &MusicPlayer::OnHorizontalSliderSliderReleased);
    connect(ui->previousMusicButton, &QPushButton::clicked, this, &MusicPlayer::OnPreviousMusicButtonClicked);
    connect(ui->nextMusicButton, &QPushButton::clicked, this, &MusicPlayer::OnNextMusicButtonClicked);
    connect(ui->musicPlaylist, &QListWidget::itemDoubleClicked, this, &MusicPlayer::OnMusicPlaylistItemDoubleClicked);
    connect(ui->musicPlaylist, &QListWidget::itemPressed, this, &MusicPlayer::OnMusicPlaylistItemPressed);
    connect(ui->musicFadeSlider, &QSlider::valueChanged, this, &MusicPlayer::OnMusicFadeSliderValueChanged);

    ui->textToSend->setWordWrapMode(QTextOption::WrapAnywhere);

    m_Player = new QMediaPlayer(this);
    m_AudioOutput = new QAudioOutput(this);
    m_Player->setAudioOutput(m_AudioOutput);

    m_FadeOutPlayer = new QMediaPlayer(this);
    m_FadeOutAudioOutput = new QAudioOutput(this);
    m_FadeOutPlayer->setAudioOutput(m_FadeOutAudioOutput);

    m_FadeOutAnimation = new QPropertyAnimation();
    m_FadeOutAnimation->setPropertyName("volume");
    m_FadeOutAnimation->setDuration(GetFadeDuration());
    m_FadeOutAnimation->setEndValue(0);

    ui->musicFadeDurationLabel->setText(QString::number(GetFadeDuration() / 1000.0f));

    connect(m_Player, &QMediaPlayer::positionChanged, this, &MusicPlayer::MusicCursorChanged);
    connect(m_Player, &QMediaPlayer::mediaStatusChanged, this, &MusicPlayer::MediaStatusChanged);

    m_AudioOutput->setVolume(1);
    ui->verticalSlider->setValue(ui->verticalSlider->maximum());

    ui->musicPlaylist->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->musicPlaylist->setDragEnabled(true);
    ui->musicPlaylist->setDefaultDropAction(Qt::MoveAction);
    ui->musicPlaylist->setAcceptDrops(true);
    ui->musicPlaylist->setDropIndicatorShown(true);

    connect(ui->musicPlaylist->model(), &QAbstractItemModel::rowsMoved, this, &MusicPlayer::PlaylistChanged);
}

void MusicPlayer::FadeOut()
{
    const float fadeDuration = GetFadeDuration();

    if (fadeDuration > 0.0f)
    {
        const float currentVolume = GetCurrentVolume();
        m_FadeOutAudioOutput->setVolume(currentVolume);

        disconnect(m_Player, &QMediaPlayer::positionChanged, this, &MusicPlayer::MusicCursorChanged);
        disconnect(m_Player, &QMediaPlayer::mediaStatusChanged, this, &MusicPlayer::MediaStatusChanged);

        std::swap(m_Player, m_FadeOutPlayer);
        std::swap(m_AudioOutput, m_FadeOutAudioOutput);

        connect(m_Player, &QMediaPlayer::positionChanged, this, &MusicPlayer::MusicCursorChanged);
        connect(m_Player, &QMediaPlayer::mediaStatusChanged, this, &MusicPlayer::MediaStatusChanged);

        m_AudioOutput->setVolume(currentVolume);

        m_FadeOutAnimation->setTargetObject(m_FadeOutAudioOutput);
        m_FadeOutAnimation->setStartValue(currentVolume);
        m_FadeOutAnimation->setDuration(fadeDuration);
        m_FadeOutAnimation->start();
    }
}

void MusicPlayer::PlayMusic(int playlistRow)
{
    FadeOut();

    if (ui->musicPlaylist->count() > 0)
    {
        int row = playlistRow % ui->musicPlaylist->count();

        QAbstractItemModel* model = ui->musicPlaylist->model();
        QModelIndex front = model->index(row, 0);

        const QString& source = model->data(front, Qt::UserRole).toString();
        QUrl wantedSource(source);

        m_Player->setSource(wantedSource);
        m_CurrentAudioRow = row;
        m_CurrentAudioName = model->data(front, Qt::DisplayRole).toString();
        ui->currentMusicLabel->setText(m_CurrentAudioName);

        OnMusicRowChanged();

        m_Player->play();
    }
}

void MusicPlayer::OnPlayMusicButtonClicked()
{
    QString source = "";
    QString name = "";
    int row = 0;

    if (ui->musicPlaylist->count() == 0)
    {
        OnAddToPlaylistButtonClicked();
    }

    if (ui->musicPlaylist->selectedItems().count() > 0)
    {
        QListWidgetItem* item = ui->musicPlaylist->selectedItems().front();
        source = item->data(Qt::UserRole).toString();
        name = item->data(Qt::DisplayRole).toString();
        row = ui->musicPlaylist->row(item);
    }
    else if (ui->musicPlaylist->count() > 0)
    {
        QAbstractItemModel* model = ui->musicPlaylist->model();
        QModelIndex front = model->index(0, 0);
        source = model->data(front, Qt::UserRole).toString();
        name = model->data(front, Qt::DisplayRole).toString();
    }

    if (!source.isEmpty())
    {
        QUrl wantedSource(source);

        if (m_Player->source() != wantedSource)
        {
            SetAudioSource(wantedSource);
            m_CurrentAudioName = name;
            m_CurrentAudioRow = row;

            OnMusicRowChanged();
        }

        m_Player->play();
    }
}


void MusicPlayer::SetAudioSource(const QUrl& source)
{
    m_IsAudioSourceBeingSet = true;

    m_Player->setSource(source);

    m_IsAudioSourceBeingSet = false;
}


void MusicPlayer::OnPauseMusicButtonClicked()
{
    if (m_Player->playbackState() != QMediaPlayer::PausedState)
    {
        m_Player->pause();
    }
    else
    {
        m_Player->play();
    }
}


void MusicPlayer::OnStopMusicButtonClicked()
{
    FadeOut();
    m_Player->stop();
    m_CurrentAudioName = "";
    ui->currentMusicLabel->setText(m_CurrentAudioName);
    ui->horizontalSlider->setSliderPosition(0);
}


void MusicPlayer::OnRemoveFromPlaylistClicked()
{
    qDeleteAll(ui->musicPlaylist->selectedItems());
}


void MusicPlayer::OnAddToPlaylistButtonClicked()
{
    const auto& data = ui->musicDataList->GetData();
    const auto& categorySelectionItemNames = ui->musicDataList->GetCategorySelectionItemNames();

    for (const auto& [category, itemNames] : categorySelectionItemNames)
    {
        const std::map<QString, QString>& categoryMap = data.at(category);
        for (const QString& itemName : itemNames)
        {
            if (!IsInPlaylist(itemName))
            {
                const QString& itemData = categoryMap.at(itemName);
                QListWidgetItem* item = new QListWidgetItem(itemName);
                item->setData(Qt::UserRole, itemData);

                AddToPlaylist(item);
            }
        }
    }
}

bool MusicPlayer::IsInPlaylist(QListWidgetItem* item) const
{
    const QString& itemName = item->data(Qt::DisplayRole).toString();
    return IsInPlaylist(itemName);
}

bool MusicPlayer::IsInPlaylist(const QString& itemName) const
{
    return Utils::Contains(*ui->musicPlaylist, itemName);
}

void MusicPlayer::AddToPlaylist(QListWidgetItem* item)
{
    if (!IsInPlaylist(item))
    {
        const QString& itemName = item->data(Qt::DisplayRole).toString();
        m_PlaylistItemList.append(itemName);
        ui->musicPlaylist->addItem(item);
    }
}

void MusicPlayer::RemoveFromPlaylist()
{
    const QList<QListWidgetItem*>& selectedItems = ui->musicPlaylist->selectedItems();
    m_PlaylistItemList.removeIf([&selectedItems](const QString& name)
    {
        return Utils::Contains(selectedItems, name);
    });

    qDeleteAll(ui->musicPlaylist->selectedItems());
}

void MusicPlayer::OnVerticalSliderValueChanged(int position)
{
    m_Player->audioOutput()->setVolume((float)position/(float)ui->verticalSlider->maximum());
}

float MusicPlayer::GetCurrentVolume() const
{
    return (float)ui->verticalSlider->value()/(float)ui->verticalSlider->maximum();
}

float MusicPlayer::GetFadeDuration() const
{
    return (float)ui->musicFadeSlider->value();
}

void MusicPlayer::OnMusicFadeSliderValueChanged(int /* position */)
{
    ui->musicFadeDurationLabel->setText(QString::number(GetFadeDuration() / 1000.0f));
}

void MusicPlayer::MusicCursorChanged(qint64 position)
{
    if (!m_IsAudioSourceBeingSet && !ui->horizontalSlider->isSliderDown())
    {
        float currentRatio = (float)position / (float)m_Player->duration();
        ui->horizontalSlider->setSliderPosition(currentRatio * ui->horizontalSlider->maximum());
    }
}

void MusicPlayer::MediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::MediaStatus::EndOfMedia)
    {
        PlayNextMusic();
    }
}

void MusicPlayer::PlayNextMusic(bool ignoreRepeat /*= false*/)
{
    if (!ignoreRepeat && ui->repeatMusicCheckbox->isChecked())
    {
        PlayMusic(m_CurrentAudioRow);
    }
    else if ((m_CurrentAudioRow + 1) < ui->musicPlaylist->count() || ui->loopMusicCheckbox->isChecked())
    {
        PlayMusic(m_CurrentAudioRow + 1);
    }
    else
    {
        OnStopMusicButtonClicked();
    }

    ui->horizontalSlider->setSliderPosition(0);
    m_Player->setPosition(0);
}

void MusicPlayer::PlayPreviousMusic(bool ignoreRepeat /*= false*/)
{
    if (!ignoreRepeat && ui->repeatMusicCheckbox->isChecked())
    {
        PlayMusic(m_CurrentAudioRow);
    }
    else if (m_CurrentAudioRow > 0)
    {
        PlayMusic(m_CurrentAudioRow - 1);
    }
    else if (ui->loopMusicCheckbox->isChecked())
    {
        PlayMusic(ui->musicPlaylist->count() - 1);
    }
    else
    {
        OnStopMusicButtonClicked();
    }

    ui->horizontalSlider->setSliderPosition(0);
    m_Player->setPosition(0);
}

void MusicPlayer::OnHorizontalSliderSliderReleased()
{
    float ratio = (float)ui->horizontalSlider->sliderPosition() / (float)ui->horizontalSlider->maximum();
    float wantedTime = ratio * (float)m_Player->duration();

    if (ratio < 1.0f)
    {
        m_Player->setPosition(fmin(wantedTime, m_Player->duration() - 1));
    }
    else
    {
        PlayNextMusic();
    }
}

void MusicPlayer::OnMusicRowChanged()
{
    SelectPlayingMusic();
}

void MusicPlayer::UnselectAllInPlaylist()
{
    for (QListWidgetItem* item : ui->musicPlaylist->selectedItems())
    {
        item->setSelected(false);
    }
}

void MusicPlayer::SelectPlayingMusic()
{
    UnselectAllInPlaylist();

    QList<QListWidgetItem*> items = ui->musicPlaylist->findItems(m_CurrentAudioName, Qt::MatchExactly);
    if (items.count() > 0)
    {
        QModelIndex currentIndex = ui->musicPlaylist->indexFromItem(items.constFirst());
        m_CurrentAudioRow = currentIndex.row();

        QItemSelectionModel* selectionModel = ui->musicPlaylist->selectionModel();
        selectionModel->select(currentIndex, QItemSelectionModel::Select);
    }
}

void MusicPlayer::OnPreviousMusicButtonClicked()
{
    PlayPreviousMusic(true);
}


void MusicPlayer::OnNextMusicButtonClicked()
{
    PlayNextMusic(true);
}

void MusicPlayer::OnMusicPlaylistItemDoubleClicked(QListWidgetItem *item)
{
    PlayMusic(ui->musicPlaylist->row(item));
}

void MusicPlayer::OnMusicPlaylistItemPressed(QListWidgetItem *item)
{
    if (QApplication::mouseButtons() & Qt::RightButton)
    {
        PlayMusic(ui->musicPlaylist->row(item));
    }
}

void MusicPlayer::SectionItemClicked(const QString& categoryName, const QString& itemName)
{
    int itemRow = 0;
    QListWidgetItem* playListitem = nullptr;

    if (IsInPlaylist(itemName))
    {
        playListitem = ui->musicPlaylist->findItems(itemName, Qt::MatchExactly).constFirst();
        itemRow = ui->musicPlaylist->indexFromItem(playListitem).row();
    }
    else
    {
        playListitem = new QListWidgetItem(itemName);

        AddToPlaylist(playListitem);

        itemRow = ui->musicPlaylist->count() - 1;
    }

    const QString& itemData = ui->musicDataList->GetData().at(categoryName).at(itemName);
    playListitem->setData(Qt::UserRole, itemData);

    PlayMusic(itemRow);
}

void MusicPlayer::SectionItemDoubleClicked(const QString& categoryName, const QString& itemName)
{
    if (!IsInPlaylist(itemName))
    {
        const QString& itemData = ui->musicDataList->GetData().at(categoryName).at(itemName);

        QListWidgetItem* playListitem = new QListWidgetItem(itemName);
        playListitem->setData(Qt::UserRole, itemData);

        AddToPlaylist(playListitem);
    }
}

void MusicPlayer::PlaylistChanged()
{
    if (m_Player->playbackState() != QMediaPlayer::StoppedState)
    {
        SelectPlayingMusic();
    }
}
