#ifndef MediaSegmentVHS_h
#define MediaSegmentVHS_h

#if ENABLE(MEDIA_SOURCE) && USE(VHS)

#include <wtf/RefCounted.h>
#include <vhs/vhs_isobmff.h>

namespace WebCore {


/**
 * MediaSegmentVHS is a RefCounted wrapper around a vhs_node_t*
 * to manage it's lifecycle.
 */
class MediaSegmentVHS : public RefCounted<MediaSegmentVHS> {

public:
    static Ref<MediaSegmentVHS> create(vhs_node_t *root) {
        return adoptRef(*new MediaSegmentVHS(root));
    }

    virtual ~MediaSegmentVHS() {
        if (m_segment) {
            vhs_destroy_tree(&m_segment);
        }
    }

    /**
     * ISOBMFF content is treated like a tree with an empty root (empty meaning
     * there is no underlying box/atom). Each child is represtented as a node,
     * with access to a box (or atom), assuming support for that box exists in
     * libvhs.
     */
    vhs_node_t * root() const { return m_segment; }

    /**
     * TODO: Move this documention to libvhs.
     * split is a dumb implemention that will divide a fragment by segments.
     * It's dumb because it's possible that a fragment contains a sidx box with
     * hints to the split and that box will be ignored.
     * vhs_split_fragment will move a moof & mdat to a newly malloc'd
     * vhs_node_t* and return the result. The original vhs_node_t* provided to
     * the split function will have two less child nodes assuming the split was
     * successful. If there is nothing left to split, NULL is returned.
     */
    vhs_node_t * split() {
        return vhs_split_fragment(&m_segment);
    }

private:
    MediaSegmentVHS(vhs_node_t *root) {
        m_segment = root;
    }

    vhs_node_t *m_segment;
};

}

#endif // ENABLE(MEDIA_SOURCE) && USE(VHS)

#endif
