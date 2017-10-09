CREATE TABLE stream (
    id INT PRIMARY KEY,
    name TEXT NOT NULL,
    url TEXT NOT NULL
);

INSERT INTO stream(id, name, url) VALUES (1, "BASSDRIVE", "http://shouthost.com.20.streams.bassdrive.com:8200");
INSERT INTO stream(id, name, url) VALUES (2, "YOU FM HESSEN", "http://hr-youfm-live.cast.addradio.de/hr/youfm/live/mp3/128/stream.mp3");
INSERT INTO stream(id, name, url) VALUES (3, "STAR FM BERLIN", "http://85.25.209.152:80/berlin.mp3");
INSERT INTO stream(id, name, url) VALUES (4, "STAR FM NÜRNBERG", "http://91.250.82.237:80/nuernberg.mp3");
INSERT INTO stream(id, name, url) VALUES (7, "94.3 RS2 BERLIN", "http://stream.rs2.de/rs2/mp3-128/");
INSERT INTO stream(id, name, url) VALUES (8, "DIRK NOTEBOOK", "http://192.168.0.90:9001/stream.mp3");
