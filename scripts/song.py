import argparse
import requests
import re
import json
import os
import random
import time
import logger
from pathlib import Path
import hashlib
from mutagen.id3 import ID3, TIT2, TALB, TPE1, TPE2, COMM, TCOM, TCON, TDRC, TRCK
from mutagen.id3 import ID3NoHeaderError
from mutagen.id3 import USLT, Encoding
from io import BytesIO


class Application(object):
    def __init__(self):
        self.logger = (
            logger.factory().console(True, logger.info).file(Path("logs")).build()
        )

        parser = argparse.ArgumentParser("Song Downloader")
        parser.add_argument("--outs", type=str, default="music")
        args = parser.parse_args()

        self.args = args
        self.callback_name = "jQuery1113030969334428388917_1750300976315"
        self.source_name = "netease"
        self.artist = "兰音Reine"

        self.domain_part = "music.gdstudio.xyz"
        self.version = "20250616"

        self.base_url = (
            f"https://music.gdstudio.xyz/api.php?callback={self.callback_name}"
        )

        self.headers = {
            "accept": "text/javascript, application/javascript, application/ecmascript, application/x-ecmascript, */*; q=0.01",
            "accept-encoding": "gzip, deflate, br, zstd",
            "accept-language": "zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6",
            "content-type": "application/x-www-form-urlencoded; charset=UTF-8",
            "origin": "https://music.gdstudio.xyz",
            "priority": "u=1, i",
            "sec-ch-ua": '"Microsoft Edge";v="137", "Chromium";v="137", "Not/A)Brand";v="24"',
            "sec-ch-ua-mobile": "?0",
            "sec-ch-ua-platform": '"Windows"',
            "sec-fetch-dest": "empty",
            "sec-fetch-mode": "cors",
            "sec-fetch-site": "same-origin",
            "user-agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/137.0.0.0 Safari/537.36 Edg/137.0.0.0",
            "x-requested-with": "XMLHttpRequest",
        }

    def exec(self):
        os.makedirs(self.args.outs, exist_ok=True)
        songs = []
        page_num = 1
        song_index = 0
        while True:
            self.logger.info(f"🔍 正在请求第 {page_num} 页...")
            search_params = {
                "callback": self.callback_name,
                "types": "search",
                "count": "20",
                "source": self.source_name,
                "name": self.artist,
                "pages": str(page_num),
            }
            response = requests.post(
                self.base_url, headers=self.headers, data=search_params
            )
            self.print_response(response)

            if response.status_code != 200:
                self.logger.error("请求失败，状态码：", response.status_code)
                break

            content = response.text.strip()
            match = re.search(
                r"^[a-zA-Z0-9_]+\(([\[\{].*[\]\}])\);$", content, re.DOTALL
            )
            if not match:
                self.logger.error("响应内容不是标准的 JSONP 格式。")
                break

            _songs = json.loads(match.group(1))
            if not _songs:
                self.logger.error("没有更多歌曲了，结束。")
                break

            for song in _songs:
                song_name = song["name"]
                artist_name = " / ".join(song["artist"])
                self.logger.info(f"🎵 歌曲名: {song_name}, 艺术家: {artist_name}")
                if self.artist not in artist_name:
                    self.logger.warning(
                        f"不是需要的歌曲! 歌曲名: {song_name}, 艺术家: {artist_name}"
                    )
                    continue
                if song_name in songs:
                    self.logger.warning(
                        f"已经下载过! 歌曲名: {song_name}, 艺术家: {artist_name}"
                    )
                    continue
                songs.append(song_name)

                delay = random.uniform(1, 10)
                self.logger.info(f"⏳ 等待 {delay:.2f} 秒后继续请求...")
                time.sleep(delay)

                song_url = self.get_song_url(song["id"])
                if not song_url:
                    self.logger.error(f"❌ 无法获取歌曲 {song_name} 的 URL")
                    continue
                self.logger.debug("⬇️ 正在下载歌曲...")
                delay = random.uniform(1, 10)
                self.logger.debug(f"⏳ 等待 {delay:.2f} 秒后继续请求...")
                time.sleep(delay)
                filename = f"{artist_name} - {song_name}.mp3"
                filename = filename.replace("/", "_").replace("\\", "_")
                self.download_song(os.path.join(self.args.outs, filename), song_url)
                song_index += 1
                self.logger.info(f"✅ 下载完成: {song_index}: {filename}")

            delay = random.uniform(1, 20)
            self.logger.error(f"⏳ 等待 {delay:.2f} 秒后继续请求...")
            time.sleep(delay)

            page_num += 1

    def get_song_url(self, song_id):
        s = self.generate_s(song_id)
        self.logger.debug(f"s={s}")
        url_params_for_play = {
            "callback": self.callback_name,
            "types": "url",
            "source": self.source_name,
            "br": "320",
            "s": s,
            "id": str(song_id),
        }

        url_response = requests.post(
            self.base_url, headers=self.headers, data=url_params_for_play
        )
        self.print_response(url_response)

        if url_response.status_code != 200:
            self.logger.error(f"状态码不是200!")
            return None

        url_match = re.search(
            r"^[a-zA-Z0-9_]+\(([\[\{].*[\]\}])\);$",
            url_response.text.strip(),
            re.DOTALL,
        )
        if not url_match:
            self.logger.error("播放地址响应格式错误。")
            return None

        url_data = json.loads(url_match.group(1))
        song_url = url_data.get("url")
        if not song_url:
            self.logger.error("未找到播放链接。")
            return None
        return song_url

    def download_song(self, filename, url):
        response = requests.get(url, stream=True)

        with open(filename, "wb") as f:
            for chunk in response.iter_content(chunk_size=1024):
                if chunk:
                    f.write(chunk)

        self.logger.info(f"✅ 已保存为：{filename}")

    def print_response(self, response):
        self.logger.debug(f"应答：")
        self.logger.debug(f"状态码：{response.status_code}")
        self.logger.debug("📡 响应头：")
        for key, value in response.headers.items():
            self.logger.debug(f"{key}: {value}")
        self.logger.debug("📡 响应体：")
        self.logger.debug(response.text)

    def generate_s(self, song_id):
        md5_hash = hashlib.md5(
            f"{self.domain_part}|{self.version}|{song_id}".encode("utf-8")
        ).hexdigest()

        return md5_hash[-8:].upper()

    def add_lyrics_to_mp3(self, mp3_file, lrc):
        audio = ID3(mp3_file)

        # 添加歌词（ID: 'USLT'）
        frame_id = "USLT::eng"
        if frame_id in audio:
            del audio[frame_id]
        # audio.delall("USLT::eng")
        audio.setall(
            "USLT",
            [USLT(encoding=Encoding.UTF8, lang="eng", format=2, type=1, text=lrc)],
        )
        audio.setall(
            "USLT",
            [USLT(encoding=Encoding.UTF8, lang="chi", format=2, type=1, text=lrc)],
        )

        # 标题
        audio["TIT2"] = TIT2(encoding=3, text="mutagen Title")
        # 专辑
        audio["TALB"] = TALB(encoding=3, text="mutagen Album")
        # 乐队
        audio["TPE2"] = TPE2(encoding=3, text="mutagen Band/Orchestra/Accompaniment")
        # 评论
        audio["COMM"] = COMM(
            encoding=3, lang="eng", desc="desc", text="mutagen comment"
        )
        # 艺术家
        audio["TPE1"] = TPE1(encoding=3, text="mutagen Artist")
        # 作曲家
        audio["TCOM"] = TCOM(encoding=3, text="mutagen Composer")
        # 流派
        audio["TCON"] = TCON(encoding=3, text="mutagen Genre")
        # 创作时间
        audio["TDRC"] = TDRC(encoding=3, text="2023")
        # 音轨
        audio["TRCK"] = TRCK(encoding=3, text="track_number")

        audio.save(mp3_file, v2_version=3)
        print("✅ 歌词已写入 MP3")


if __name__ == "__main__":
    app = Application()
    app.exec()
