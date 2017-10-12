CREATE TABLE stream (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    url TEXT NOT NULL
);

CREATE TABLE music (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    url TEXT NOT NULL
);

INSERT INTO stream(name, url) VALUES ("BASSDRIVE", "http://shouthost.com.20.streams.bassdrive.com:8200");
INSERT INTO stream(name, url) VALUES ("YOU FM HESSEN", "http://hr-youfm-live.cast.addradio.de/hr/youfm/live/mp3/128/stream.mp3");
INSERT INTO stream(name, url) VALUES ("STAR FM BERLIN", "http://85.25.209.152:80/berlin.mp3");
INSERT INTO stream(name, url) VALUES ("STAR FM NÜRNBERG", "http://91.250.82.237:80/nuernberg.mp3");
INSERT INTO stream(name, url) VALUES ("STAR FM ALTERNATIVE", "http://85.25.43.55:80/alternative.mp3");
INSERT INTO stream(name, url) VALUES ("STAR FM BLUES", "http://85.25.43.55:80/blues.mp3");
INSERT INTO stream(name, url) VALUES ("STAR FM CLASSIC ROCK", "http://85.25.43.55:80/rock_classics.mp3");
INSERT INTO stream(name, url) VALUES ("STAR FM FROM HELL", "http://85.25.43.55:80/hell.mp3");
INSERT INTO stream(name, url) VALUES ("STAR FM HOT TOP", "http://85.25.209.152:80/hot_top.mp3");
INSERT INTO stream(name, url) VALUES ("94.3 RS2 BERLIN", "http://stream.rs2.de/rs2/mp3-128/");
INSERT INTO stream(name, url) VALUES ("94.3 RS2 80ER", "http://stream.rs2.de/rs2-80er/mp3-128/internetradio/");
INSERT INTO stream(name, url) VALUES ("94.3 RS2 90ER", "http://stream.rs2.de/rs2-90er/mp3-128/internetradio/");
INSERT INTO stream(name, url) VALUES ("94.3 RS2 RELAX", "http://stream.rs2.de/rs2-relax/mp3-128/internetradio/");
INSERT INTO stream(name, url) VALUES ("NOTEBOOK", "http://192.168.0.90:9001/stream.mp3");
INSERT INTO stream(name, url) VALUES ("ODROID", "http://192.168.0.10:9001/stream.mp3");
