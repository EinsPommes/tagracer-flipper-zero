# TagRacer - Multiplayer NFC Game for Flipper Zero

TagRacer is an exciting multiplayer NFC tag scanning game for the Flipper Zero device. Race against other players to scan NFC tags and score points in real-time!

## 🎮 Game Components

### Flipper Zero App
- Real-time NFC tag scanning
- Live score updates
- Game timer with notifications
- Anti-cheat mechanisms
- Seamless server communication

### Bridge Software
- Connects Flipper Zero to server
- Serial communication handling
- WebSocket support for real-time updates
- Automatic reconnection
- Message queue system

### Server Backend
- Flask + Flask-SocketIO
- RESTful API endpoints
- SQLite database with SQLAlchemy
- Real-time game state management
- Score tracking and validation

### Web Interface
- Vue.js frontend
- Live scoreboard
- Real-time activity feed
- Statistics and heatmaps
- Achievement system

## 🚀 Getting Started

### Prerequisites
- Flipper Zero device
- Python 3.8+
- Node.js 14+
- USB connection for bridge software

### Installation

1. **Flipper Zero App**
```bash
# Clone to your Flipper Zero apps folder
git clone https://github.com/yourusername/tagracer-flipper-zero.git
cd flipper-tagracer

# Build using fbt
./fbt
```

2. **Bridge Software**
```bash
cd bridge
pip install -r requirements.txt
python main.py
```

3. **Server**
```bash
cd server
pip install -r requirements.txt
python app.py
```

4. **Web Interface**
```bash
cd web
npm install
npm run dev
```

## 🎯 How to Play

1. Start the bridge software and server
2. Launch TagRacer on your Flipper Zero
3. Press OK to start a new game
4. Find and scan NFC tags within the time limit
5. Watch your score update in real-time
6. Compete for the highest score!

## 🏆 Features

- **Real-time Multiplayer**: Compete with others in real-time
- **Anti-Cheat System**: Prevents tag scanning abuse
- **Achievement System**: Unlock achievements as you play
- **Statistics**: Track your progress and view heatmaps
- **Live Updates**: See scores and activity in real-time
- **Cross-Platform**: Web interface works on any device

## 🛠️ Development

### Project Structure
```
tagracer-flipper-zero/
├── application/       # Flipper Zero application
├── bridge/           # Bridge software
├── server/           # Backend server
└── web/             # Web interface
```

### Contributing
1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Flipper Zero team for the amazing platform
- All contributors and players
- The open-source community

## 📧 Contact

Your Name - [@yourusername](https://twitter.com/yourusername)

Project Link: [https://github.com/yourusername/tagracer-flipper-zero](https://github.com/yourusername/tagracer-flipper-zero)
