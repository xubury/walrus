// main.js
const { app, BrowserWindow, session } = require('electron')
const path = require('path')
const isDev = require('electron-is-dev');
const os = require('os')
const waitPort = require('wait-port');
require("../server.js")

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
        mainWindow.webContents.openDevTools()
        await session.defaultSession.loadExtension(reactDevToolsPath)
    } 
    const params = {
      host: 'localhost',
      port: 8080,
    };
    waitPort(params)
      .then(({ open, ipVersion }) => { mainWindow.loadURL('http://localhost:8080/'); })
      .catch((err) => { console.err(`An unknown error occured while waiting for the port: ${err}`); });
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
