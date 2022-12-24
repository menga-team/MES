import sys
import os
import serial
import traceback
from PyQt5 import QtGui
from PyQt5.QtCore import Qt, QTimer, QStringListModel
from PyQt5.QtGui import QKeyEvent
from PyQt5.QtWidgets import QApplication, QWidget, QPushButton, QHBoxLayout, QVBoxLayout, QTabWidget, QGridLayout, QTextEdit, QComboBox, QCompleter, QLineEdit


class Tee:
    def __init__(self, variable, terminal):
        self.variable = variable
        self.terminal = terminal

    def write(self, data):
        self.variable.append(data)
        self.terminal.write(data)
        self.terminal.flush()
        window.updateConsoleOutput()

    def writelines(self, data):
        self.variable.extend(data)
        self.terminal.writelines(data)
        self.terminal.flush()
        window.updateConsoleOutput()

    def flush(self, *args, **kwargs):
        pass

# Create a list to store the output
output = []

# Redirect standard output to both the list and the terminal
sys.stdout = Tee(output, sys.stdout)

class MainWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.buttonState = [0] * 32
        self.controllerState = [0] * 4

        self.buttons = [0] * 36
        self.button_args = [
            [0, "A\nRight", Qt.Key_A],
            [1, "W\nUp", Qt.Key_W],
            [2, "D\nLeft", Qt.Key_D],
            [3, "S\ndown", Qt.Key_S],
            [4, "X\nselect", Qt.Key_X],
            [5, "Z\nstart", Qt.Key_Z],
            [6, "E\nB", Qt.Key_E],
            [7, "Q\nA", Qt.Key_Q],
            [8, "F\nRight", Qt.Key_F],
            [9, "T\nUp", Qt.Key_T],
            [10, "H\nLeft", Qt.Key_H],
            [11, "G\ndown", Qt.Key_G],
            [12, "B\nselect", Qt.Key_B],
            [13, "V\nstart", Qt.Key_V],
            [14, "Y\nB", Qt.Key_Y],
            [15, "R\nA", Qt.Key_R],
            [16, "J\nRight", Qt.Key_J],
            [17, "I\nUp", Qt.Key_I],
            [18, "L\nLeft", Qt.Key_L],
            [19, "K\ndown", Qt.Key_K],
            [20, "Comma\nselect", Qt.Key_Comma],
            [21, "M\nstart", Qt.Key_M],
            [22, "O\nB", Qt.Key_O],
            [23, "U\nA", Qt.Key_U],
            [24, "Right\nRight", Qt.Key_Right],
            [25, "Up\nUp", Qt.Key_Up],
            [26, "Left\nLeft", Qt.Key_Left],
            [27, "down\ndown", Qt.Key_Down],
            [28, "]\nselect", Qt.Key_BracketRight],
            [29, "[\nstart", Qt.Key_BracketLeft],
            [30, "=\nB", Qt.Key_Slash],
            [31, "-\nA", Qt.Key_Minus],
            [32, "One\ntoggle", Qt.Key_1],
            [33, "two\ntoggle", Qt.Key_2],
            [34, "three\ntoggle", Qt.Key_3],
            [35, "four\ntoggle", Qt.Key_4],

        ]

        for number, name, shortcut in self.button_args:
            button = QPushButton(name)
            button.setCheckable(True)
            # button.setShortcut(shortcut)
            # if 32 > number:
            #     button.clicked.connect(lambda: self.setButton(new_number, button.isChecked()))
            # else:
            #     button.clicked.connect(lambda: self.setController(new_number - 32, button.isChecked()))
            self.buttons[number] = button

        self.actions = [
            lambda: self.setButton(0, self.buttons[0].isChecked),
            lambda: self.setButton(1, self.buttons[1].isChecked),
            lambda: self.setButton(2, self.buttons[2].isChecked),
            lambda: self.setButton(3, self.buttons[3].isChecked),
            lambda: self.setButton(4, self.buttons[4].isChecked),
            lambda: self.setButton(5, self.buttons[5].isChecked),
            lambda: self.setButton(6, self.buttons[6].isChecked),
            lambda: self.setButton(7, self.buttons[7].isChecked),
            lambda: self.setButton(8, self.buttons[8].isChecked),
            lambda: self.setButton(9, self.buttons[9].isChecked),
            lambda: self.setButton(10, self.buttons[10].isChecked),
            lambda: self.setButton(11, self.buttons[11].isChecked),
            lambda: self.setButton(12, self.buttons[12].isChecked),
            lambda: self.setButton(13, self.buttons[13].isChecked),
            lambda: self.setButton(14, self.buttons[14].isChecked),
            lambda: self.setButton(15, self.buttons[15].isChecked),
            lambda: self.setButton(16, self.buttons[16].isChecked),
            lambda: self.setButton(17, self.buttons[17].isChecked),
            lambda: self.setButton(18, self.buttons[18].isChecked),
            lambda: self.setButton(19, self.buttons[19].isChecked),
            lambda: self.setButton(20, self.buttons[20].isChecked),
            lambda: self.setButton(21, self.buttons[21].isChecked),
            lambda: self.setButton(22, self.buttons[22].isChecked),
            lambda: self.setButton(23, self.buttons[23].isChecked),
            lambda: self.setButton(24, self.buttons[24].isChecked),
            lambda: self.setButton(25, self.buttons[25].isChecked),
            lambda: self.setButton(26, self.buttons[26].isChecked),
            lambda: self.setButton(27, self.buttons[27].isChecked),
            lambda: self.setButton(28, self.buttons[28].isChecked),
            lambda: self.setButton(29, self.buttons[29].isChecked),
            lambda: self.setButton(30, self.buttons[30].isChecked),
            lambda: self.setButton(31, self.buttons[31].isChecked),
            lambda: self.setController(0, self.buttons[32].isChecked),
            lambda: self.setController(1, self.buttons[33].isChecked),
            lambda: self.setController(2, self.buttons[34].isChecked),
            lambda: self.setController(3, self.buttons[35].isChecked),
        ]

        for function, button in zip(self.actions, self.buttons):
            button.clicked.connect(function)

        # Set up tab widget
        self.tabWidget = QTabWidget()
        self.tabWidget.addTab(QWidget(), "Controller 1")
        self.tabWidget.addTab(QWidget(), "Controller 2")
        self.tabWidget.addTab(QWidget(), "Controller 3")
        self.tabWidget.addTab(QWidget(), "Controller 4")

        # Set up layout for each tab
        for i in range(4):
            buttonwidget = QWidget()
            tab = self.tabWidget.widget(i)
            tabLayout = QVBoxLayout()
            tabLayout.addWidget(buttonwidget)
            # Set up button layout
            # button = buttons[i+9]
            tabLayout.addWidget(self.buttons[32 + i])

            for d in range(8):
                self.buttons[i*8+d].setParent(buttonwidget)

            self.buttons[i*8+0].setGeometry(150,200, 50, 50)
            self.buttons[i*8+1].setGeometry(200,150, 50, 50)
            self.buttons[i*8+2].setGeometry(250,200, 50, 50)
            self.buttons[i*8+3].setGeometry(200,250, 50, 50)

            self.buttons[i*8+4].setGeometry(350,200, 50, 50)
            self.buttons[i*8+5].setGeometry(400,200, 50, 50)
            self.buttons[i*8+6].setGeometry(500,200, 50, 50)
            self.buttons[i*8+7].setGeometry(550,200, 50, 50)

            tab.setLayout(tabLayout)

        # Set up main layout
        mainLayout = QGridLayout()
        mainLayout.addWidget(self.tabWidget, 1, 0)

        self.model = QStringListModel()
        self.model.setStringList([f"/dev/{i}" for i in os.listdir("/dev/")])
        self.completer = QCompleter()
        self.completer.setModel(self.model)

        self.device = QLineEdit()
        self.device.setText("/dev/ACM0")
        self.device.setCompleter(self.completer)
        mainLayout.addWidget(self.device, 0, 0)

        # self.device_combobox = QComboBox()
        # self.device_combobox.addItems([f"/dev/{i}" for i in os.listdir("/dev/")])

        self.toggle_checkablity = QPushButton("toggle checkability")
        self.toggle_checkablity.setCheckable(True)
        self.toggle_checkablity.setChecked(True)
        self.toggle_checkablity.clicked.connect(lambda:[i.setCheckable(self.toggle_checkablity.isChecked()) for i in self.buttons])
        mainLayout.addWidget(self.toggle_checkablity, 0, 1)

        self.textEdit = QTextEdit()
        self.textEdit.setReadOnly(True)
        mainLayout.addWidget(self.textEdit, 1, 1)

        self.autosendButton = QPushButton("Autosend to Arduino")
        self.autosendButton.setCheckable(True)
        self.autosendButton.clicked.connect(lambda : print(f'{"enabled" if self.autosendButton.isChecked() else "disabled"} autosend'))
        mainLayout.addWidget(self.autosendButton, 2, 1)

        p = QPushButton("Send to Arduino")
        p.clicked.connect(self.sendToArduino)
        mainLayout.addWidget(p, 2, 0)

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.updateConsoleOutput)

        # Set timer interval to 100 milliseconds
        self.timer.start(100)
        self.setLayout(mainLayout)

    def keyPressEvent(self, e: QKeyEvent):
        for i in zip(self.buttons, [x[2] for x in self.button_args]):
            # print(i)
            if e.key() == i[1]:
                i[0].setChecked(True)

    def keyReleaseEvent(self, e: QKeyEvent):
        for i in zip(self.buttons, [x[2] for x in self.button_args]):
            # print(i)
            if e.key() == i[1]:
                i[0].setChecked(False)

    def setButton(self, index, status):
        newline = '\n'
        print(f"button {self.button_args[index][1].split(newline)} set to {status()}")
        self.buttonState[index] = int(status())
        if self.autosendButton.isChecked():
            self.sendToArduino()

    def setController(self, index, status):
        print(f"controller {index} set to {status()}")
        self.controllerState[index] = int(status())

    def sendToArduino(self):

        print("Sending : <", end="")
        print("".join([str(i) for i in self.buttonState]), end="")
        print("".join([str(i) for i in self.controllerState]), end="")
        # for state in self.buttonState:
        #     print(bytes([int(state)]), end="")
        # for state in self.controllerState:
        #     print(bytes([int(state)]), end="")
        print(">")

        # Open serial connection
        try:
            with serial.Serial(self.device.text(), 9600) as ser:
                # Send start byte
                ser.write(b"<")

                # Send button state
                # for state in :
                ser.write(bytes(self.buttonState))
                ser.write(bytes(self.controllerState))

                # Send controller state
                # for state in self.controllerState:
                #     ser.write(bytes([int(state)]))

                # Send end byte
                ser.write(b">")
        except Exception as e:
            tb = traceback.format_exc()
            print(tb)

    def updateConsoleOutput(self):
        self.textEdit.setText(''.join(output))


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())
