// main.js
const { app, BrowserWindow, session } = require('electron')
const path = require('path')
const isDev = require('electron-is-dev');
const os = require('os')

 // on windows
 const reactDevToolsPath = path.join(
   os.homedir(),
   '/AppData/Local/Google/Chrome/User Data/Default/Extensions/pdcpmagijalfljmkmjngeonclgbbannb/0.2.5386.0_0'
 )

async function createWindow () {
    const mainWindow = new BrowserWindow({
        width: 800,
        height: 600,
    })

    if (isDev) {
        mainWindow.loadURL('http://localhost:8080/');
        mainWindow.webContents.openDevTools()
        await session.defaultSession.loadExtension(reactDevToolsPath)
    } else {
        mainWindow.loadFile('./build/index.html');
    }
}

app.whenReady().then(() => {
    createWindow()
    app.on('activate', function () {
        if (BrowserWindow.getAllWindows().length === 0) createWindow()
    })
})

app.on('window-all-closed', function () {
    if (process.platform !== 'darwin') app.quit()
})
