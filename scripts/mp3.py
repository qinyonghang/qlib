import argparse
import os
from io import BytesIO
from mutagen.id3 import ID3, USLT, Encoding as IEncoding
import logger


default_logger = logger.factory().console().build()


def add_lyrics_to_audio(output_file: str, input_file: str, lrc_file: str, is_eng=False):
    """
    支持 MP3 和 MP4（AAC/M4A）格式，添加歌词到音频文件中。
    """
    ext = os.path.splitext(input_file)[1].lower()

    with open(lrc_file, "r", encoding="utf-8") as f:
        lyrics_text = f.read()

    lang = "chi_sim" if not is_eng else "eng"

    if ext == ".mp3":
        # 处理 MP3 文件
        try:
            tags = ID3(input_file)
        except Exception:
            tags = ID3()

        print("✅ 检测到 MP3 格式")

        print(f"✅ 标题: {tags['TIT2']}")
        print(f"✅ 歌手: {tags['TPE1']}")
        print(f"✅ 专辑: {tags['TALB']}")
        print(f"✅ 流派: {tags['TCON']}")
        print(f"✅ 乐队: {tags['TPE2']}")
        print(f"✅ 评论: {tags['COMM']}")
        print(f"✅ 作曲家: {tags['TCOM']}")
        print(f"✅ 创作时间: {tags['TDRC']}")
        print(f"✅ 音轨: {tags['TRCK']}")

        # 删除旧歌词
        if tags.getall(f"USLT::{lang}"):
            print(f"❌ {lang} 歌词已存在，正在删除...")
            tags.delall(f"USLT::{lang}")

        # 添加新歌词
        tags.add(
            USLT(
                encoding=IEncoding.UTF8,
                lang=lang,
                format=0,
                type=1,
                desc="",
                text=lyrics_text,
            )
        )

        # 写入文件
        output_data = BytesIO()
        tags.save(output_data, v2_version=3)

        with open(output_file, "wb") as f:
            f.write(output_data.getvalue())

    elif ext in (".m4a", ".mp4", ".m4b"):
        # 处理 M4A / MP4 文件
        from mutagen.mp4 import MP4, MP4Tags

        print("✅ 检测到 MP4/M4A 格式")

        audio = MP4(input_file)
        tags = audio.tags if audio.tags else MP4Tags()

        print(f"✅ 标题: {tags.get('©nam', ['未知'])[0]}")
        print(f"✅ 歌手: {tags.get('©ART', ['未知'])[0]}")
        print(f"✅ 专辑: {tags.get('©alb', ['未知'])[0]}")
        print(f"✅ 流派: {tags.get('©gen', ['未知'])[0]}")
        print(f"✅ 作曲家: {tags.get('©wrt', ['未知'])[0]}")
        print(f"✅ 评论: {tags.get('©cmt', ['未知'])[0]}")
        print(f"✅ 创作时间: {tags.get('©day', ['未知'])[0]}")

        track = tags.get("trkn", [(0, 0)])
        print(f"✅ 音轨: {track[0][0]}/{track[0][1]}")
        print(f"✅ 乐队: {tags.get('aART', ['未知'])[0]}")

        # Apple 的歌词标签是 "\xa9lyr"
        if "\xa9lyr" in tags:
            print("❌ 已有歌词，正在覆盖...")
        tags["\xa9lyr"] = lyrics_text

        # 保存到新文件
        audio.save(output_file)
    else:
        raise ValueError(f"不支持的音频格式: {ext}")

    print(f"✅ 已保存带歌词的新文件为: {output_file}")


def show_mp3_tags(file):
    tags = ID3(file)
    for key in tags.keys():
        if "APIC" in key:
            apic_tag = tags[key]
            default_logger.info(
                f"🖼️ {key}\n\tencoding={apic_tag.encoding}, mime={apic_tag.mime}, type={apic_tag.type}, desc={apic_tag.desc}"
            )
            if apic_tag.mime == "image/jpeg":
                filename = "APIC.jpg"
            elif apic_tag.mime == "image/png":
                filename = "APIC.png"
            elif apic_tag.mime == "image/gif":
                filename = "APIC.gif"
            else:
                filename = "APIC.unknown"
            with open(filename, "wb") as f:
                f.write(tags[key].data)
            default_logger.info(f"✅ 已保存专辑图片为: {filename}")
        elif key.startswith("USLT:"):
            default_logger.info(f"🏷️ {key}\n\n{tags[key]}")
        else:
            default_logger.info(f"🏷️ {key}\n\t{tags[key]}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser("添加歌词")
    parser.add_argument("song", help="歌曲文件路径")
    parser.add_argument("lyrics", nargs="?", default=None, help="歌词文件路径")
    parser.add_argument("out", nargs="?", default=None, help="输出歌曲文件路径")
    args = parser.parse_args()
    default_logger.info(args)
    # add_lyrics_to_audio(args.out, args.song, args.lyrics)
    show_mp3_tags(args.song)
