/* gcc -o element element.c `pkg-config --cflags --libs gstreamer-1.0` */
#include <gst/gst.h>

int main( int argc, char **argv)
{
    GstElement *pipeline, *source, *sink; // 파이프라인, 소스, 싱크 요소 선언
    GstBus *bus; // 버스를 통해 메시지를 전달받기 위한 버스 선언
    GstMessage *msg; // 오류 메시지 또는 EOS 메시지를 받기 위한 메시지 선언
    GstStateChangeReturn ret; // 상태 전환 결과를 저장할 변수 선언

    gst_init(&argc, &argv); // GStreamer 라이브러리 초기화

    // "videotestsrc"와 "autovideosink" 엘리먼트를 생성
    source = gst_element_factory_make("videotestsrc", "source");
    sink = gst_element_factory_make("autovideosink", "sink");

    // 새로운 파이프라인 생성
    pipeline = gst_pipeline_new("test-pipeline");

    // 엘리먼트가 제대로 생성되었는지 확인
    if (!pipeline || !source || !sink) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    // 파이프라인에 소스와 싱크 요소 추가
    gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);

    // 소스와 싱크를 연결 (미디어 데이터가 소스에서 싱크로 전달되도록)
    if (gst_element_link(source, sink) != TRUE) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline); // 파이프라인 메모리 해제
        return -1;
    }

    // "videotestsrc"의 패턴을 설정 (0번 패턴은 기본 패턴)
    g_object_set(source, "pattern", 0, NULL);

    // 파이프라인의 상태를 "PLAYING" 상태로 변경
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    // 상태 변경에 실패했는지 확인
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline); // 파이프라인 메모리 해제
        return -1;
    }

    // 파이프라인의 버스를 얻어옴 (이벤트와 메시지를 받기 위함)
    bus = gst_element_get_bus(pipeline);

    // 메시지 대기: 오류 메시지 또는 스트림 끝 메시지를 기다림
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    // 메시지 해제
    gst_message_unref(msg);

    // 버스 메모리 해제
    gst_object_unref(bus);

    // 파이프라인을 "NULL" 상태로 설정하여 정지하고 리소스 해제
    gst_element_set_state(pipeline, GST_STATE_NULL);

    // 파이프라인 메모리 해제
    gst_object_unref(pipeline);

    return 0;
}
