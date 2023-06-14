#include <stdio.h>
#include <libgen.h>
#include <getopt.h>

#include <ebml/EbmlHead.h>
#include <ebml/StdIOCallback.h>

#include <matroska/KaxSegment.h>
#include <matroska/KaxInfo.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxCues.h>

#include <ebml/EbmlVersion.h>
#include <matroska/KaxVersion.h>

extern "C" {
#include "mbestformat.h"
#include "util.h"
}

static const struct option options[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "out", required_argument, NULL, 'o' },
};

static const char *applet = "mbest2mkv";
static const wchar_t *lapplet = L"mbest2mkv";
static const char *bcwt_codec_id = "V_BCWT";

/* Use a 1ms tick. */
static const int timestamp_scale_1ms = 1000000;

static void usage()
{
	printf("Usage: %s [-o FILE] SCRIPT\n", applet);
	printf( "Combine seperate files into a matroska file from SCRIPT.\n"
		"\n"
		"\t-o, --out   Output filename. If not set, SCRIPT.mkv will be used.\n"
		"\n"
		"SCRIPT is a file that contains paths to seperated files and other\n"
		"information to help generate output matroska file. A sample SCRIPT format as follows.\n"
		"\n```\n"
		"25\n"
		"3\n"
		"1920\n"
		"1080\n"
		"*0.bst\n"
		"-1.bst\n"
		" 2.bst\n"
		"```\n\n"
		"The first line 25 is frame rate, second line 3 total frame count, 1920 and 1080 pixel\n"
		"width and height.\n"
		"Last 3 lines are input files in order. Note that every path should\n"
		"be prefixed by '*', '-', or ' ', meaning\n"
		"\t'*' This frame is a key frame.\n"
		"\t'-' This frame is discardable.\n"
		"\t'-' This frame is just normal.\n"
		"accordingly.\n"
		);
}

using namespace libebml;
using namespace libmatroska;

static void kax_info_set(KaxInfo &info,
		unsigned int timestamp_scale, float duration)
{
	/* Historically timestamps in Matroska were mistakenly called timecodes. */
	*(static_cast<EbmlUInteger*>(&GetChild<KaxTimecodeScale>(info))) = timestamp_scale;
	*(static_cast<EbmlFloat*>(&GetChild<KaxDuration>(info))) = duration;

	const std::size_t sz =
		50 + EbmlCodeVersion.length() + KaxCodeVersion.length();
	wchar_t *ma = new wchar_t[sz];
	swprintf(ma, sz, L"libebml %s + libmatroska %s",
			EbmlCodeVersion.c_str(), KaxCodeVersion.c_str());

	*((EbmlUnicodeString*)&GetChild<KaxMuxingApp>(info)) = ma;
	*((EbmlUnicodeString*)&GetChild<KaxWritingApp>(info)) = lapplet;
}

static void kax_track_entry_set(KaxTrackEntry &entry, unsigned int number,
		unsigned int uid, enum track_type type, const char *codec_id,
		bool lacing, unsigned int default_duration)
{
	*(static_cast<EbmlUInteger*>(&GetChild<KaxTrackNumber>(entry))) = number;
	*(static_cast<EbmlUInteger*>(&GetChild<KaxTrackUID>(entry))) = uid;
	*(static_cast<EbmlUInteger*>(&GetChild<KaxTrackType>(entry))) = type;
	*(static_cast<EbmlString*>(&GetChild<KaxCodecID>(entry))) = codec_id;
	*(static_cast<EbmlUInteger*>(&GetChild<KaxTrackDefaultDuration>(entry))) = default_duration;
	entry.EnableLacing(lacing);
}

static void kax_track_entry_add_video(KaxTrackEntry &entry,
                unsigned int width, unsigned int height)
{
	KaxTrackVideo &v = GetChild<KaxTrackVideo>(entry);
	*(static_cast<EbmlUInteger*>(&GetChild<KaxVideoPixelWidth>(v))) = width;
	*(static_cast<EbmlUInteger*>(&GetChild<KaxVideoPixelHeight>(v))) = height;
}

// C style good, C++ lambda bad.
static bool databuffer_free(const DataBuffer &db)
{
	free((void*)db.Buffer());
	return true;
}


int main(int argc, char *const argv[])
{
	const char *out_filename = NULL;
	char *in_filename = NULL;

	int c;
	while ((c = getopt_long(argc, argv, "ho:", options, NULL)) != -1) {
		switch (c) {
		case 'h':
			usage();
			return 0;
		case 'o':
			out_filename = optarg;
			break;
		}
	}

	in_filename = argv[optind];

	if (!in_filename) {
		M2M_FATAL("SCRIPT required.");
		return 1;
	}

	if (!out_filename) {
		char *of = (char*)malloc(strlen(in_filename) + 5);
		strcpy(of, basename(in_filename));
		size_t i = strlen(of);
		of[i] = '.';
		of[i+1] = 'm'; of[i+2] = 'k'; of[i+3] = 'v'; of[i+4] = '\0';
		out_filename = of;
	}

	struct mbest_format_t *mf = mbest_format_init(in_filename);
	if (!mf) {
		M2M_FATAL("Could not init format.");
		return 1;
	}

	StdIOCallback outfile(out_filename, MODE_CREATE);
	filepos_t segment_size, info_size, tracks_size, cluster_size;

	/*
	 * Default is matroska format(At least the time the program is written),
	 * just use it.
	 */
	EbmlHead head;
	head.Render(outfile, true);

	KaxSegment segment;

	KaxInfo &info = GetChild<KaxInfo>(segment);
	kax_info_set(info, timestamp_scale_1ms, (1000.0/mf->fps)*mf->frames);

	// 5 byte placeholder for whole video file sizelength.
	// TODO: increase sizelength.
	segment_size = segment.WriteHead(outfile, 5, true);
	info_size = info.Render(outfile);

	KaxTracks &tracks = GetChild<KaxTracks>(segment);

	KaxTrackEntry &bcwt_video_track = GetChild<KaxTrackEntry>(tracks);
	kax_track_entry_set(bcwt_video_track, 1, 1, track_video,
			bcwt_codec_id, false, timestamp_scale_1ms/mf->fps*1000);
        kax_track_entry_add_video(bcwt_video_track, mf->width, mf->height);

	tracks_size = tracks.Render(outfile);

	KaxCluster *cur_cluster = new KaxCluster;
	cur_cluster->SetParent(segment);
	cur_cluster->InitTimecode(0, 1);
	// We must have a cue to render a cluster.
	KaxCues cue;
	uint64 timestamp = 0, cur_cluster_timestamp = 0;

	cluster_size = 0;

	for (size_t i = 0; i < mf->frames; i++) {
		timestamp = i * (1000/mf->fps);

		char *buf;
		size_t len;
		mbest_read_frame(mf, i, &buf, &len);
		if (!buf) {
			// IO error, most likely a wrong filename.
			perror(mbest_frame_info_filename(mf->frame_info[i]));
			exit(1);
		}

		DataBuffer *d = new DataBuffer((binary*)buf, len, &databuffer_free);
		KaxBlockBlob *b = new KaxBlockBlob(BLOCK_BLOB_ALWAYS_SIMPLE);
		b->SetParent(*cur_cluster);
		b->AddFrameAuto(bcwt_video_track, i*1000/mf->fps, *d);
		((KaxSimpleBlock&)*b).SetKeyframe(mbest_is_keyframe(mf, i));
		((KaxSimpleBlock&)*b).SetDiscardable(mbest_is_discardable(mf, i));
		cur_cluster->AddBlockBlob(b);

		// Each cluster holds at least 5 seconds of frames,
		// this is basically sane and won't cause block local timestamp overflow.
		if (timestamp - cur_cluster_timestamp > 5000 || i == mf->frames-1) {
			cluster_size += cur_cluster->Render(outfile, cue);

			KaxCluster *new_cluster = new KaxCluster();
			new_cluster->SetParent(segment);
			cur_cluster_timestamp = timestamp;
			new_cluster->InitTimecode(cur_cluster_timestamp, 1);
			// this also frees simpleblocks held by cluster.
			delete cur_cluster;
			cur_cluster = new_cluster;
		}
	}

	mbest_format_free(mf);

	// rewrite the placeholder.
	if (segment.ForceSize(info_size + tracks_size + cluster_size))
		segment.OverwriteHead(outfile);
	else { // This is unlikely to happen actually.
		M2M_FATAL("Could not force size.");
		outfile.close();
		return 1;
	}
	outfile.close();
	return 0;
}
