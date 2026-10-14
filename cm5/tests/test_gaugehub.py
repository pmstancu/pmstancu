import csv
import datetime as dt

from gaugehub.logger import CsvLogger
from gaugehub.protocol import CHANNELS, Frame, Hello, parse_line
from gaugehub.sources import SimSource

LINE = (
    '{"t":1234,"v":{"ect":88.100,"eot":95.000,"iat":41.200,"egt":null,"boost":1.200,'
    '"afr":20.500,"vbat":14.100},"l":{"ect":0,"eot":0,"iat":0,"egt":0,"boost":1,"afr":0,"vbat":0},'
    '"f":{"ect":0,"eot":0,"iat":0,"egt":255,"boost":0,"afr":0,"vbat":0}}'
)


def test_parse_frame():
    f = parse_line(LINE)
    assert isinstance(f, Frame)
    assert f.device_ms == 1234
    assert f.values["ect"] == 88.1
    assert f.values["egt"] is None
    assert f.levels["boost"] == 1
    assert f.faults["egt"] == 255
    assert set(f.values) == set(CHANNELS)


def test_parse_hello_and_bytes():
    h = parse_line(b'{"hello":"esp32-gauges","fw":"0.1.0"}\n')
    assert h == Hello(device="esp32-gauges", firmware="0.1.0")


def test_parse_rejects_noise():
    for bad in ["", "ESP-ROM:esp32s3-20210327", "{not json", "[1,2]", '{"v":{}}', '{"t":"x","v":{}}']:
        assert parse_line(bad) is None


def test_parse_missing_channel_and_bad_types():
    f = parse_line('{"t":1,"v":{"ect":true,"egt":"hot"}}')
    assert f.values["ect"] is None and f.values["egt"] is None and f.values["vbat"] is None
    assert f.levels["ect"] == 0


def test_logger_writes_daily_file(tmp_path):
    log = CsvLogger(tmp_path, min_interval_s=0)
    frame = parse_line(LINE)
    day1 = dt.datetime(2026, 10, 14, 23, 59, 59)
    day2 = dt.datetime(2026, 10, 15, 0, 0, 1)
    assert log.write(frame, now=day1)
    assert log.write(frame, now=day2)
    log.close()

    rows = list(csv.reader((tmp_path / "2026-10-14.csv").open()))
    assert rows[0] == CsvLogger.HEADER
    assert rows[1][CsvLogger.HEADER.index("ect")] == "88.100"
    assert rows[1][CsvLogger.HEADER.index("egt")] == ""
    assert rows[1][CsvLogger.HEADER.index("egt_flt")] == "255"
    assert (tmp_path / "2026-10-15.csv").exists()


def test_logger_rate_limit(tmp_path):
    log = CsvLogger(tmp_path, min_interval_s=10)
    frame = parse_line(LINE)
    assert log.write(frame)
    assert not log.write(frame)
    log.close()


def test_simulator_frames_are_complete():
    sim = SimSource(on_frame=lambda f: None)
    for t in (0, 30, 200):
        f = sim.sample(t)
        assert set(f.values) == set(CHANNELS)
        assert all(v is not None for v in f.values.values())
