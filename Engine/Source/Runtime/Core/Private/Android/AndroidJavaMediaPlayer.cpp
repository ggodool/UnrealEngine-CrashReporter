// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AndroidJavaMediaPlayer.h"
#include "AndroidApplication.h"

#if UE_BUILD_SHIPPING
// always clear any exceptions in SHipping
#define CHECK_JNI_RESULT(Id) if (Id == 0) { JEnv->ExceptionClear(); }
#else
#define CHECK_JNI_RESULT(Id) \
if (Id == 0) \
{ \
	if (bIsOptional) { JEnv->ExceptionClear(); } \
	else { JEnv->ExceptionDescribe(); checkf(Id != 0, TEXT("Failed to find " #Id)); } \
}
#endif

static jfieldID FindField(JNIEnv* JEnv, jclass Class, const ANSICHAR* FieldName, const ANSICHAR* FieldType, bool bIsOptional)
{
	jfieldID Field = Class == NULL ? NULL : JEnv->GetFieldID(Class, FieldName, FieldType);
	CHECK_JNI_RESULT(Field);
	return Field;
}

FJavaAndroidMediaPlayer::FJavaAndroidMediaPlayer(bool vulkanRenderer)
	: FJavaClassObject(GetClassName(), "(Z)V", vulkanRenderer)
	, GetDurationMethod(GetClassMethod("getDuration", "()I"))
	, ResetMethod(GetClassMethod("reset", "()V"))
	, GetCurrentPositionMethod(GetClassMethod("getCurrentPosition", "()I"))
	, IsLoopingMethod(GetClassMethod("isLooping", "()Z"))
	, IsPlayingMethod(GetClassMethod("isPlaying", "()Z"))
	, SetDataSourceURLMethod(GetClassMethod("setDataSourceURL", "(Ljava/lang/String;)Z"))
	, SetDataSourceFileMethod(GetClassMethod("setDataSource", "(Ljava/lang/String;JJ)Z"))
	, SetDataSourceAssetMethod(GetClassMethod("setDataSource", "(Landroid/content/res/AssetManager;Ljava/lang/String;JJ)Z"))
	, PrepareMethod(GetClassMethod("prepare", "()V"))
	, SeekToMethod(GetClassMethod("seekTo", "(I)V"))
	, SetLoopingMethod(GetClassMethod("setLooping", "(Z)V"))
	, ReleaseMethod(GetClassMethod("release", "()V"))
	, GetVideoHeightMethod(GetClassMethod("getVideoHeight", "()I"))
	, GetVideoWidthMethod(GetClassMethod("getVideoWidth", "()I"))
	, SetVideoEnabledMethod(GetClassMethod("setVideoEnabled", "(Z)V"))
	, SetAudioEnabledMethod(GetClassMethod("setAudioEnabled", "(Z)V"))
	, GetVideoLastFrameDataMethod(GetClassMethod("getVideoLastFrameData", "()Ljava/nio/Buffer;"))
	, StartMethod(GetClassMethod("start", "()V"))
	, PauseMethod(GetClassMethod("pause", "()V"))
	, StopMethod(GetClassMethod("stop", "()V"))
	, GetVideoLastFrameMethod(GetClassMethod("getVideoLastFrame", "(I)Z"))
	, GetAudioTracksMethod(GetClassMethod("GetAudioTracks", "()[Lcom/epicgames/ue4/MediaPlayer14$AudioTrackInfo;"))
	, GetCaptionTracksMethod(GetClassMethod("GetCaptionTracks", "()[Lcom/epicgames/ue4/MediaPlayer14$CaptionTrackInfo;"))
	, GetVideoTracksMethod(GetClassMethod("GetVideoTracks", "()[Lcom/epicgames/ue4/MediaPlayer14$VideoTrackInfo;"))
	, DidResolutionChangeMethod(GetClassMethod("didResolutionChange", "()Z"))
{
	bTrackInfoSupported = FAndroidMisc::GetAndroidBuildVersion() >= 16;

	if (bTrackInfoSupported)
	{
		SelectTrackMethod = GetClassMethod("selectTrack", "(I)V");
	}

	JNIEnv* JEnv = FAndroidApplication::GetJavaEnv();

	// get field IDs for AudioTrackInfo class members
	jclass localAudioTrackInfoClass = FAndroidApplication::FindJavaClass("com/epicgames/ue4/MediaPlayer14$AudioTrackInfo");
	AudioTrackInfoClass = (jclass)JEnv->NewGlobalRef(localAudioTrackInfoClass);
	JEnv->DeleteLocalRef(localAudioTrackInfoClass);
	AudioTrackInfo_Index = FindField(JEnv, AudioTrackInfoClass, "Index", "I", false);
	AudioTrackInfo_MimeType = FindField(JEnv, AudioTrackInfoClass, "MimeType", "Ljava/lang/String;", false);
	AudioTrackInfo_DisplayName = FindField(JEnv, AudioTrackInfoClass, "DisplayName", "Ljava/lang/String;", false);
	AudioTrackInfo_Language = FindField(JEnv, AudioTrackInfoClass, "Language", "Ljava/lang/String;", false);
	AudioTrackInfo_Channels = FindField(JEnv, AudioTrackInfoClass, "Channels", "I", false);
	AudioTrackInfo_SampleRate = FindField(JEnv, AudioTrackInfoClass, "SampleRate", "I", false);

	// get field IDs for CaptionTrackInfo class members
	jclass localCaptionTrackInfoClass = FAndroidApplication::FindJavaClass("com/epicgames/ue4/MediaPlayer14$CaptionTrackInfo");
	CaptionTrackInfoClass = (jclass)JEnv->NewGlobalRef(localCaptionTrackInfoClass);
	JEnv->DeleteLocalRef(localCaptionTrackInfoClass);
	CaptionTrackInfo_Index = FindField(JEnv, CaptionTrackInfoClass, "Index", "I", false);
	CaptionTrackInfo_MimeType = FindField(JEnv, CaptionTrackInfoClass, "MimeType", "Ljava/lang/String;", false);
	CaptionTrackInfo_DisplayName = FindField(JEnv, CaptionTrackInfoClass, "DisplayName", "Ljava/lang/String;", false);
	CaptionTrackInfo_Language = FindField(JEnv, CaptionTrackInfoClass, "Language", "Ljava/lang/String;", false);

	// get field IDs for VideoTrackInfo class members
	jclass localVideoTrackInfoClass = FAndroidApplication::FindJavaClass("com/epicgames/ue4/MediaPlayer14$VideoTrackInfo");
	VideoTrackInfoClass = (jclass)JEnv->NewGlobalRef(localVideoTrackInfoClass);
	JEnv->DeleteLocalRef(localVideoTrackInfoClass);
	VideoTrackInfo_Index = FindField(JEnv, VideoTrackInfoClass, "Index", "I", false);
	VideoTrackInfo_MimeType = FindField(JEnv, VideoTrackInfoClass, "MimeType", "Ljava/lang/String;", false);
	VideoTrackInfo_DisplayName = FindField(JEnv, VideoTrackInfoClass, "DisplayName", "Ljava/lang/String;", false);
	VideoTrackInfo_Language = FindField(JEnv, VideoTrackInfoClass, "Language", "Ljava/lang/String;", false);
	VideoTrackInfo_BitRate = FindField(JEnv, VideoTrackInfoClass, "BitRate", "I", false);
	VideoTrackInfo_Width = FindField(JEnv, VideoTrackInfoClass, "Width", "I", false);
	VideoTrackInfo_Height = FindField(JEnv, VideoTrackInfoClass, "Height", "I", false);
	VideoTrackInfo_FrameRate = FindField(JEnv, VideoTrackInfoClass, "FrameRate", "F", false);
}

int32 FJavaAndroidMediaPlayer::GetDuration()
{
	return CallMethod<int32>(GetDurationMethod);
}

void FJavaAndroidMediaPlayer::Reset()
{
	CallMethod<void>(ResetMethod);
}

void FJavaAndroidMediaPlayer::Stop()
{
	CallMethod<void>(StopMethod);
}

int32 FJavaAndroidMediaPlayer::GetCurrentPosition()
{
	int32 position = CallMethod<int32>(GetCurrentPositionMethod);
	return position;
}

bool FJavaAndroidMediaPlayer::IsLooping()
{
	return CallMethod<bool>(IsLoopingMethod);
}

bool FJavaAndroidMediaPlayer::IsPlaying()
{
	return CallMethod<bool>(IsPlayingMethod);
}

bool FJavaAndroidMediaPlayer::SetDataSource(const FString & Url)
{
	return CallMethod<bool>(SetDataSourceURLMethod, GetJString(Url));
}

bool FJavaAndroidMediaPlayer::SetDataSource(const FString& MoviePathOnDevice, int64 offset, int64 size)
{
	return CallMethod<bool>(SetDataSourceFileMethod, GetJString(MoviePathOnDevice), offset, size);
}

bool FJavaAndroidMediaPlayer::SetDataSource(jobject AssetMgr, const FString& AssetPath, int64 offset, int64 size)
{
	return CallMethod<bool>(SetDataSourceAssetMethod, AssetMgr, GetJString(AssetPath), offset, size);
}

bool FJavaAndroidMediaPlayer::Prepare()
{
	// This can return an exception in some cases (URL without internet, for example)
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	JEnv->CallVoidMethod(Object, PrepareMethod.Method);
	if (JEnv->ExceptionCheck())
	{
		JEnv->ExceptionDescribe();
		JEnv->ExceptionClear();
		return false;
	}
	return true;
}

void FJavaAndroidMediaPlayer::SeekTo(int32 Milliseconds)
{
	CallMethod<void>(SeekToMethod, Milliseconds);
}

void FJavaAndroidMediaPlayer::SetLooping(bool Looping)
{
	CallMethod<void>(SetLoopingMethod, Looping);
}

void FJavaAndroidMediaPlayer::Release()
{
	CallMethod<void>(ReleaseMethod);
}

int32 FJavaAndroidMediaPlayer::GetVideoHeight()
{
	return CallMethod<int32>(GetVideoHeightMethod);
}

int32 FJavaAndroidMediaPlayer::GetVideoWidth()
{
	return CallMethod<int32>(GetVideoWidthMethod);
}

void FJavaAndroidMediaPlayer::SetVideoEnabled(bool enabled /*= true*/)
{
	CallMethod<void>(SetVideoEnabledMethod, enabled);
}

void FJavaAndroidMediaPlayer::SetAudioEnabled(bool enabled /*= true*/)
{
	CallMethod<void>(SetAudioEnabledMethod, enabled);
}

bool FJavaAndroidMediaPlayer::GetVideoLastFrameData(void* & outPixels, int64 & outCount)
{
	jobject buffer = CallMethod<jobject>(GetVideoLastFrameDataMethod);
	if (nullptr != buffer)
	{
		JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
		outPixels = JEnv->GetDirectBufferAddress(buffer);
		outCount = JEnv->GetDirectBufferCapacity(buffer);
		// we don't reduce the buffer refcount because it is still owned by the Java class, and will be reused.
	}
	if (nullptr == buffer || nullptr == outPixels || 0 == outCount)
	{
		return false;
	}
	return true;
}

void FJavaAndroidMediaPlayer::Start()
{
	CallMethod<void>(StartMethod);
}

void FJavaAndroidMediaPlayer::Pause()
{
	CallMethod<void>(PauseMethod);
}

bool FJavaAndroidMediaPlayer::DidResolutionChange()
{
	return CallMethod<bool>(DidResolutionChangeMethod);
}

bool FJavaAndroidMediaPlayer::GetVideoLastFrame(int32 destTexture)
{
	// This can return an exception in some cases
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	bool Result = JEnv->CallBooleanMethod(Object, GetVideoLastFrameMethod.Method, destTexture);
	if (JEnv->ExceptionCheck())
	{
		JEnv->ExceptionDescribe();
		JEnv->ExceptionClear();
		return false;
	}
	return Result;
}

FName FJavaAndroidMediaPlayer::GetClassName()
{
	if (FAndroidMisc::GetAndroidBuildVersion() >= 14)
	{
		return FName("com/epicgames/ue4/MediaPlayer14");
	}
	else
	{
		return FName("");
	}
}

bool FJavaAndroidMediaPlayer::SelectTrack(int32 index)
{
	if (!bTrackInfoSupported)
	{
		// Just assume it worked
		return true;
	}

	// This can return an exception in some cases
	JNIEnv*	JEnv = FAndroidApplication::GetJavaEnv();
	JEnv->CallVoidMethod(Object, SelectTrackMethod.Method, index);
	if (JEnv->ExceptionCheck())
	{
		JEnv->ExceptionDescribe();
		JEnv->ExceptionClear();
		return false;
	}
	return true;
}

bool FJavaAndroidMediaPlayer::GetAudioTracks(TArray<FAudioTrack>& AudioTracks)
{
	AudioTracks.Empty();

	jobjectArray TrackArray = CallMethod<jobjectArray>(GetAudioTracksMethod);
	if (nullptr != TrackArray)
	{
		bool bIsOptional = false;
		JNIEnv* JEnv = FAndroidApplication::GetJavaEnv();
		jsize ElementCount = JEnv->GetArrayLength(TrackArray);

		for (int Index = 0; Index < ElementCount; ++Index)
		{
			jobject Track = JEnv->GetObjectArrayElement(TrackArray, Index);

			int32 AudioTrackIndex = AudioTracks.AddDefaulted();
			FAudioTrack& AudioTrack = AudioTracks[AudioTrackIndex];

			AudioTrack.Index = (int32)JEnv->GetIntField(Track, AudioTrackInfo_Index);

			jstring jsMimeType = (jstring)JEnv->GetObjectField(Track, AudioTrackInfo_MimeType);
			CHECK_JNI_RESULT(jsMimeType);
			const char * nativeMimeType = JEnv->GetStringUTFChars(jsMimeType, 0);
			AudioTrack.MimeType = FString(nativeMimeType);
			JEnv->ReleaseStringUTFChars(jsMimeType, nativeMimeType);
			JEnv->DeleteLocalRef(jsMimeType);

			jstring jsDisplayName = (jstring)JEnv->GetObjectField(Track, AudioTrackInfo_DisplayName);
			CHECK_JNI_RESULT(jsDisplayName);
			const char * nativeDisplayName = JEnv->GetStringUTFChars(jsDisplayName, 0);
			AudioTrack.DisplayName = FString(nativeDisplayName);
			JEnv->ReleaseStringUTFChars(jsDisplayName, nativeDisplayName);
			JEnv->DeleteLocalRef(jsDisplayName);

			jstring jsLanguage = (jstring)JEnv->GetObjectField(Track, AudioTrackInfo_Language);
			CHECK_JNI_RESULT(jsLanguage);
			const char * nativeLanguage = JEnv->GetStringUTFChars(jsLanguage, 0);
			AudioTrack.Language = FString(nativeLanguage);
			JEnv->ReleaseStringUTFChars(jsLanguage, nativeLanguage);
			JEnv->DeleteLocalRef(jsLanguage);

			AudioTrack.Channels = (int32)JEnv->GetIntField(Track, AudioTrackInfo_Channels);
			AudioTrack.SampleRate = (int32)JEnv->GetIntField(Track, AudioTrackInfo_SampleRate);
		}
		JEnv->DeleteGlobalRef(TrackArray);

		return true;
	}
	return false;
}

bool FJavaAndroidMediaPlayer::GetCaptionTracks(TArray<FCaptionTrack>& CaptionTracks)
{
	CaptionTracks.Empty();

	jobjectArray TrackArray = CallMethod<jobjectArray>(GetCaptionTracksMethod);
	if (nullptr != TrackArray)
	{
		bool bIsOptional = false;
		JNIEnv* JEnv = FAndroidApplication::GetJavaEnv();
		jsize ElementCount = JEnv->GetArrayLength(TrackArray);

		for (int Index = 0; Index < ElementCount; ++Index)
		{
			jobject Track = JEnv->GetObjectArrayElement(TrackArray, Index);

			int32 CaptionTrackIndex = CaptionTracks.AddDefaulted();
			FCaptionTrack& CaptionTrack = CaptionTracks[CaptionTrackIndex];

			CaptionTrack.Index = (int32)JEnv->GetIntField(Track, CaptionTrackInfo_Index);

			jstring jsMimeType = (jstring)JEnv->GetObjectField(Track, CaptionTrackInfo_MimeType);
			CHECK_JNI_RESULT(jsMimeType);
			const char * nativeMimeType = JEnv->GetStringUTFChars(jsMimeType, 0);
			CaptionTrack.MimeType = FString(nativeMimeType);
			JEnv->ReleaseStringUTFChars(jsMimeType, nativeMimeType);
			JEnv->DeleteLocalRef(jsMimeType);

			jstring jsDisplayName = (jstring)JEnv->GetObjectField(Track, CaptionTrackInfo_DisplayName);
			CHECK_JNI_RESULT(jsDisplayName);
			const char * nativeDisplayName = JEnv->GetStringUTFChars(jsDisplayName, 0);
			CaptionTrack.DisplayName = FString(nativeDisplayName);
			JEnv->ReleaseStringUTFChars(jsDisplayName, nativeDisplayName);
			JEnv->DeleteLocalRef(jsDisplayName);

			jstring jsLanguage = (jstring)JEnv->GetObjectField(Track, CaptionTrackInfo_Language);
			CHECK_JNI_RESULT(jsLanguage);
			const char * nativeLanguage = JEnv->GetStringUTFChars(jsLanguage, 0);
			CaptionTrack.Language = FString(nativeLanguage);
			JEnv->ReleaseStringUTFChars(jsLanguage, nativeLanguage);
			JEnv->DeleteLocalRef(jsLanguage);
		}
		JEnv->DeleteGlobalRef(TrackArray);

		return true;
	}
	return false;
}

bool FJavaAndroidMediaPlayer::GetVideoTracks(TArray<FVideoTrack>& VideoTracks)
{
	VideoTracks.Empty();

	jobjectArray TrackArray = CallMethod<jobjectArray>(GetVideoTracksMethod);
	if (nullptr != TrackArray)
	{
		bool bIsOptional = false;
		JNIEnv* JEnv = FAndroidApplication::GetJavaEnv();
		jsize ElementCount = JEnv->GetArrayLength(TrackArray);

		for (int Index = 0; Index < ElementCount; ++Index)
		{
			jobject Track = JEnv->GetObjectArrayElement(TrackArray, Index);

			int32 VideoTrackIndex = VideoTracks.AddDefaulted();
			FVideoTrack& VideoTrack = VideoTracks[VideoTrackIndex];

			VideoTrack.Index = (int32)JEnv->GetIntField(Track, VideoTrackInfo_Index);

			jstring jsMimeType = (jstring)JEnv->GetObjectField(Track, VideoTrackInfo_MimeType);
			CHECK_JNI_RESULT(jsMimeType);
			const char * nativeMimeType = JEnv->GetStringUTFChars(jsMimeType, 0);
			VideoTrack.MimeType = FString(nativeMimeType);
			JEnv->ReleaseStringUTFChars(jsMimeType, nativeMimeType);
			JEnv->DeleteLocalRef(jsMimeType);

			jstring jsDisplayName = (jstring)JEnv->GetObjectField(Track, VideoTrackInfo_DisplayName);
			CHECK_JNI_RESULT(jsDisplayName);
			const char * nativeDisplayName = JEnv->GetStringUTFChars(jsDisplayName, 0);
			VideoTrack.DisplayName = FString(nativeDisplayName);
			JEnv->ReleaseStringUTFChars(jsDisplayName, nativeDisplayName);
			JEnv->DeleteLocalRef(jsDisplayName);

			jstring jsLanguage = (jstring)JEnv->GetObjectField(Track, VideoTrackInfo_Language);
			CHECK_JNI_RESULT(jsLanguage);
			const char * nativeLanguage = JEnv->GetStringUTFChars(jsLanguage, 0);
			VideoTrack.Language = FString(nativeLanguage);
			JEnv->ReleaseStringUTFChars(jsLanguage, nativeLanguage);
			JEnv->DeleteLocalRef(jsLanguage);

			VideoTrack.BitRate = (int32)JEnv->GetIntField(Track, VideoTrackInfo_BitRate);
			VideoTrack.Dimensions = FIntPoint((int32)JEnv->GetIntField(Track, VideoTrackInfo_Width), (int32)JEnv->GetIntField(Track, VideoTrackInfo_Height));
			VideoTrack.FrameRate = JEnv->GetFloatField(Track, VideoTrackInfo_FrameRate);
		}
		JEnv->DeleteGlobalRef(TrackArray);

		return true;
	}
	return false;
}
