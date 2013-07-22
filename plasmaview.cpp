/* This file is part of PlasmaView.
 *
 * PlasmaView is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PlasmaView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Gneral Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PlasmaView.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "plasmaview.h"

#include <QApplication>
#include <QDockWidget>
#include <QTreeWidget>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QDir>
#include <QSettings>
#include <ResManager/plResManager.h>
#include <PRP/Object/plSceneObject.h>
#include <PRP/Geometry/plDrawableSpans.h>
#include <PRP/plSceneNode.h>
#include "plasma_gles.h"

#define plStringToQString(x)  QString::fromUtf8((x).cstr())
#define qStringToPlString(x)  plString((x).toUtf8().constData())

PlasmaView::PlasmaView()
    : m_resMgr(0)
{
    setWindowTitle("Plasma Viewer");

    QDockWidget *treeDock = new QDockWidget("Age Contents", this);
    treeDock->setAllowedAreas(Qt::AllDockWidgetAreas);

    m_objectTree = new QTreeWidget(treeDock);
    m_objectTree->setHeaderHidden(true);
    m_objectTree->setRootIsDecorated(true);
    m_objectTree->setIconSize(QSize(16, 16));
    connect(m_objectTree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            SLOT(selectObject(QTreeWidgetItem*,QTreeWidgetItem*)));

    treeDock->setWidget(m_objectTree);
    addDockWidget(Qt::LeftDockWidgetArea, treeDock);

    m_render = new PlasmaGLWidget(this);
    setCentralWidget(m_render);

    QToolBar *mainTbar = addToolBar("Main Toolbar");
    QAction *aOpen = mainTbar->addAction(QIcon::fromTheme("document-open"), "&Load Age");
    aOpen->setShortcut(QKeySequence::Open);
    connect(aOpen, SIGNAL(triggered()), SLOT(onOpenAge()));

    mainTbar->addSeparator();
    QAction *aPoints = mainTbar->addAction(QIcon(":/res/view-points.png"), "&Points");
    aPoints->setShortcut(QKeySequence("F1"));
    aPoints->setCheckable(true);
    QAction *aWire = mainTbar->addAction(QIcon(":/res/view-wire.png"), "&Wireframe");
    aWire->setShortcut(QKeySequence("F2"));
    aWire->setCheckable(true);
    QAction *aFlat = mainTbar->addAction(QIcon(":/res/view-flat.png"), "&Flat");
    aFlat->setShortcut(QKeySequence("F3"));
    aFlat->setCheckable(true);
    QAction *aTextured = mainTbar->addAction(QIcon(":/res/view-textured.png"), "&Textured");
    aTextured->setShortcut(QKeySequence("F4"));
    aTextured->setCheckable(true);
    aTextured->setChecked(true);

    resize(800, 600);
}

PlasmaView::~PlasmaView()
{
    delete m_resMgr;
}

void PlasmaView::onOpenAge()
{
    QSettings settings("PlasmaShop", "PlasmaView");
    QString lastPath = settings.value("LastOpenDir", QDir::currentPath()).toString();

    QString name = QFileDialog::getOpenFileName(this, "Load Plasma Age", lastPath,
                                                "Plasma Age (*.age);;All Files (*)");
    if (!name.isEmpty()) {
        settings.setValue("LastOpenDir", QFileInfo(name).absolutePath());
        loadAge(name);
    }
}

void PlasmaView::loadAge(const QString &filename)
{
    m_objectTree->clear();
    delete m_resMgr;
    m_resMgr = new plResManager;

    QString ageFile = QDir::toNativeSeparators(QDir::current().absoluteFilePath(filename));
    plPageInfo *lastPage = 0;

    QProgressDialog progress("Please Wait...", QString(), 0, 0);
    progress.setWindowModality(Qt::WindowModal);
    progress.setAutoClose(false);
    m_resMgr->SetProgressFunc([&progress, &lastPage](plPageInfo *page, size_t curObj, size_t maxObjs) {
        if (page != lastPage) {
            progress.setLabelText(QString("Loading %1...")
                    .arg(page ? plStringToQString(page->getPage()) : "<Unknown Page>"));
            progress.setMaximum(maxObjs);
            lastPage = page;
        }
        progress.setValue(curObj);
    });
    progress.show();
    qApp->processEvents();

    plAgeInfo *age = m_resMgr->ReadAge(ageFile.toUtf8().constData(), true);
    for (size_t pg = 0; pg < age->getNumPages(); ++pg) {
        plLocation pageLoc = age->getPageLoc(pg, m_resMgr->getVer());
        std::vector<plKey> keys = m_resMgr->getKeys(pageLoc, kSceneNode);
        if (keys.size() == 0)
            // No scene node here!
            continue;

        PlasmaTreeWidgetItem *page_item = new PlasmaTreeWidgetItem(m_objectTree,
                QStringList(plStringToQString(age->getPage(pg).fName)));
        page_item->setIcon(0, QIcon(":/res/page.png"));
        page_item->setLocation(pageLoc);

        if (keys.size() > 1) {
            QMessageBox::critical(this, "Bad Node",
                QString("PRPs should have exactly 1 scene node, but %1 had %2!")
                .arg(plStringToQString(age->getPage(pg).fName)).arg(keys.size()));
            continue;
        }

        plSceneNode *node = plSceneNode::Convert(keys[0]->getObj());
        keys = node->getSceneObjects();
        if (keys.size() == 0) {
            // Scene node found, but no objects in it
            delete page_item;
            continue;
        }

        foreach (const plKey &key, keys) {
            plSceneObject *obj = plSceneObject::Convert(key->getObj());
            PlasmaTreeWidgetItem *obj_item = new PlasmaTreeWidgetItem(page_item,
                    QStringList(plStringToQString(key->getName())));
            obj_item->setLocation(pageLoc);

            if (obj->getDrawInterface().Exists())
                obj_item->setIcon(0, QIcon(":/res/sceneobj.png"));
            else
                obj_item->setIcon(0, QIcon(":/res/sim.png"));
        }
        page_item->sortChildren(0, Qt::AscendingOrder);
    }
}

void PlasmaView::selectObject(QTreeWidgetItem *current, QTreeWidgetItem *)
{
    if (current == 0)
        return;

    PlasmaTreeWidgetItem *item = static_cast<PlasmaTreeWidgetItem *>(current);
    if (item->location() == m_currentLocation)
        return;

    // HACK
    m_render->clear();
    std::vector<plKey> keys = m_resMgr->getKeys(item->location(), kDrawableSpans);
    foreach (const plKey &key, keys) {
        plDrawableSpans *spans = plDrawableSpans::Convert(key->getObj());
        m_render->addGeometry(spans);
    }

    m_render->updateGL();
    m_currentLocation = item->location();
}
