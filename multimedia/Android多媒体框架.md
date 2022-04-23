# Android多媒体框架

## MediaScanner

### 基本概念

### 负责扫描多媒体文件的信息并存进数据库中，然后通过MediaProvider提供接口使用。例如music应用程序中的歌曲专辑名、歌曲时长等都是由其扫描得到。

#### 普通文件的扫描流程

##### 扫描请求的接收者MediaScannerReceiver

其继承于BroadcastReceiver，通过静态注册方式进行注册：

```xml
<receiver android:name="com.android.providers.media.MediaReceiver"
            android:exported="true">
            <intent-filter>
                <action android:name="android.intent.action.BOOT_COMPLETED" />
            </intent-filter>
            <intent-filter>
                <action android:name="android.intent.action.LOCALE_CHANGED" />
            </intent-filter>
            <intent-filter>
                <action android:name="android.intent.action.PACKAGE_FULLY_REMOVED" />
                <action android:name="android.intent.action.PACKAGE_DATA_CLEARED" />
                <data android:scheme="package" />
            </intent-filter>
            <intent-filter>
                <action android:name="android.intent.action.MEDIA_MOUNTED" />
                <data android:scheme="file" />
            </intent-filter>
            <intent-filter>
                <action android:name="android.intent.action.MEDIA_SCANNER_SCAN_FILE" />
                <data android:scheme="file" />
            </intent-filter>
        </receiver>
```

MediaSannerReceiver接受的扫描请求广播如下：

- BOOT_COMPLETED：系统启动完成；

- MEDIA_MOUNTED：外部存储器被加载时；

- MEDIA_SCANNER_SCAN_FILE：接受到对某个文件进行扫描的请求的时候调用scanFile方法进行扫描。

- LOCALE_CHANGED：语言改变时；

- PACKAGE_FULLY_REMOVED：应用被完全删除时发出；

- PACKAGE_DATA_CLEARED：清除一个应用程序的数据时发出广播

  ​

接收到广播后响应如下：

```java
final String action = intent.getAction();
        // ESWIN_AOSP_ENHANCEMENT
        Log.i(TAG, "onReceive " + intent);
        // end of ESWIN_AOSP_ENHANCEMENT
        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            // Register our idle maintenance service
            IdleService.scheduleIdlePass(context);

        } else {
            // All other operations are heavier-weight, so redirect them through
            // service to ensure they have breathing room to finish
            intent.setComponent(new ComponentName(context, MediaService.class));
            MediaService.enqueueWork(context, intent);
        }
```

当接收到开机启动的广播消息时，会设置触发条件来发布作业，一旦满足条件，将会调用onStartJob方法，初始化MediaProvider，进而唤起ModernMediaScanner，如下：

```java
    @Override
    public boolean onStartJob(JobParameters params) {
        mSignal = new CancellationSignal();
        new Thread(() -> {
            try (ContentProviderClient cpc = getContentResolver()
                 	//初始化MediaProvider
                    .acquireContentProviderClient(MediaStore.AUTHORITY)) {
                ((MediaProvider) cpc.getLocalContentProvider()).onIdleMaintenance(mSignal);
            } catch (OperationCanceledException ignored) {
            }
            jobFinished(params, false);
        }).start();
        return true;
    }
```

如果是其他的消息，由于其他操作都是重量级的，因此通过服务将其重定向，以确保它们有空间来完成。

##### 扫描处理的后台处理者是MediaService

*服务如何起来的，待处理*

此服务主要负责后台的扫描，其主要逻辑如下：

```java
    protected void onHandleWork(Intent intent) {
        Trace.beginSection(intent.getAction());
        if (Log.isLoggable(TAG, Log.INFO)) {
            Log.i(TAG, "Begin " + intent);
        }
        try {
            switch (intent.getAction()) {
                case Intent.ACTION_LOCALE_CHANGED: {
                    onLocaleChanged();
                    break;
                }
                case Intent.ACTION_PACKAGE_FULLY_REMOVED:
                case Intent.ACTION_PACKAGE_DATA_CLEARED: {
                    final String packageName = intent.getData().getSchemeSpecificPart();
                    onPackageOrphaned(packageName);
                    break;
                }
                case Intent.ACTION_MEDIA_SCANNER_SCAN_FILE: {
                    onScanFile(this, intent.getData());
                    break;
                }
                case Intent.ACTION_MEDIA_MOUNTED: {
                    onMediaMountedBroadcast(this, intent);
                    break;
                }
                case ACTION_SCAN_VOLUME: {
                    final MediaVolume volume = intent.getParcelableExtra(EXTRA_MEDIAVOLUME);
                    int reason = intent.getIntExtra(EXTRA_SCAN_REASON, REASON_DEMAND);
                    onScanVolume(this, volume, reason);
                    break;
                }
                default: {
                    Log.w(TAG, "Unknown intent " + intent);
                    break;
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "Failed operation " + intent, e);
        } finally {
            if (Log.isLoggable(TAG, Log.INFO)) {
                Log.i(TAG, "End " + intent);
            }
            Trace.endSection();
        }
    }
```

从代码中可以看出，是根据Intent中携带的信息来选择处理流程，如果关键字为Intent.ACTION_MEDIA_SCANNER_SCAN_FILE则说明需要扫描的为文件，会调用onScanFile来对路径进行扫描，如果关键字为ACTION_SCAN_VOLUME，则代表需要扫描的是卷，调用的方法为onScanVolume。以下以扫描卷为例：

```java
    public static void onScanVolume(Context context, MediaVolume volume, int reason)
            throws IOException {
        final String volumeName = volume.getName();
        UserHandle owner = volume.getUser();
        if (owner == null) {
            // Can happen for the internal volume
            owner = context.getUser();
        }
        // 在扫描外部存储之前先扫描内部
        if (!MediaStore.VOLUME_INTERNAL.equals(volumeName)) {
            onScanVolume(context, MediaVolume.fromInternal(), reason);
            RingtoneManager.ensureDefaultRingtones(context);
        }

        // 解析与此卷相关的所有广播意图的Uri。
        final Uri broadcastUri;
        if (!MediaStore.VOLUME_INTERNAL.equals(volumeName)) {
            broadcastUri = Uri.fromFile(volume.getPath());
        } else {
            broadcastUri = null;
        }

        try (ContentProviderClient cpc = context.getContentResolver()
                .acquireContentProviderClient(MediaStore.AUTHORITY)) {
            final MediaProvider provider = ((MediaProvider) cpc.getLocalContentProvider());
          	//附加卷名
            provider.attachVolume(volume, /* validate */ true);

            final ContentResolver resolver = ContentResolver.wrap(cpc.getLocalContentProvider());

            ContentValues values = new ContentValues();
            values.put(MediaStore.MEDIA_SCANNER_VOLUME, volumeName);
          	//将卷名插入数据库中
            Uri scanUri = resolver.insert(MediaStore.getMediaScannerUri(), values);

            if (broadcastUri != null) {
                context.sendBroadcastAsUser(
                        new Intent(Intent.ACTION_MEDIA_SCANNER_STARTED, broadcastUri), owner);
            }

            if (MediaStore.VOLUME_INTERNAL.equals(volumeName)) {
                for (File dir : FileUtils.getVolumeScanPaths(context, volumeName)) {
                  ////ModernMediaScanner开始扫描文件
                    provider.scanDirectory(dir, reason);
                }
            } else {
                provider.scanDirectory(volume.getPath(), reason);
            }

            resolver.delete(scanUri, null, null);

        } finally {
            if (broadcastUri != null) {
                context.sendBroadcastAsUser(
                        new Intent(Intent.ACTION_MEDIA_SCANNER_FINISHED, broadcastUri), owner);
            }
        }
    }
```

##### 扫描文件的ModernMediaScanner类

先看一下ModernMediaScanner类的初始化过程：

```java
public ModernMediaScanner(Context context) {
        mContext = context;
  		//实例化DrmManagerClient，以通过DRM框架来访问DRM代理
        mDrmClient = new DrmManagerClient(context);

        // 查询DRM中注册的DRM代理信息
        for (DrmSupportInfo info : mDrmClient.getAvailableDrmSupportInfo()) {
          	//检索支持MIME类型的迭代器对象
            Iterator<String> mimeTypes = info.getMimeTypeIterator();
            while (mimeTypes.hasNext()) {
                mDrmMimeTypes.add(mimeTypes.next());
            }
        }
    }
```

当MediaService判定为扫描卷时，最终会调用ModernMediaScanner中的scanDirectory方法，此方法会首先初始化Scan类，代码如下：

```java
public Scan(File root, int reason, @Nullable String ownerPackage)
                throws FileNotFoundException {
            Trace.beginSection("ctor");

            mClient = mContext.getContentResolver()
                    .acquireContentProviderClient(MediaStore.AUTHORITY);
            mResolver = ContentResolver.wrap(mClient.getLocalContentProvider());

            mRoot = root;
            mReason = reason;
			//判断给定目录是否在外设根目录下
            if (FileUtils.contains(Environment.getStorageDirectory(), root)) {
              //获取外设的相关信息，包括设备名、设备所有者、设备挂载目录以及设备id
              mVolume = MediaVolume.fromStorageVolume(FileUtils.getStorageVolume(mContext, root));
            } else {
              	//获取内部设备的设备名
                mVolume = MediaVolume.fromInternal();
            }
            mVolumeName = mVolume.getName();
  			//获取给定设备的url
            mFilesUri = MediaStore.Files.getContentUri(mVolumeName);
            mSignal = new CancellationSignal();
			//返回给定卷的最新世代值
            mStartGeneration = MediaStore.getGeneration(mResolver, mVolumeName);
            mSingleFile = mRoot.isFile();
            mOwnerPackage = ownerPackage;
            mExcludeDirs = new ArrayList<>();
  			//客户定制
            if (ESWIN_AOSP_ENHANCEMENG_ENABLE) {
              //将文件扩展名加入到mFileExts这个集合中
                mFileExts.addAll(Arrays.asList(VIDEO_EXTS.split(";")));
                mFileExts.addAll(Arrays.asList(AUDIO_EXTS.split(";")));
                mFileExts.addAll(Arrays.asList(IMAGE_EXTS.split(";")));
                mFileExts.addAll(Arrays.asList(APK_EXTS.split(";")));
                Log.d(TAG, "The file extensions: " + mFileExts);
            }

            Trace.endSection();
        }
```

若Scan实例成功创建，调用Scan的run方法，代码如下：

```java
@Override
        public void run() {
          	//将该实例加入至数组中
            addActiveScan(this);
            try {
                runInternal();
            } finally {
                removeActiveScan(this);
            }
        }
```

之后执行runInternal方法，代码如下：

```java
private void runInternal() {
  			//获取系统启动后的时间
            final long startTime = SystemClock.elapsedRealtime();

            // First, scan everything that should be visible under requested
            // location, tracking scanned IDs along the way
  			//扫描在请求位置下应该可见的所有内容，并跟踪扫描的ID
            walkFileTree();

            // Second, reconcile all items known in the database against all the
            // items we scanned above
  			//将数据库中已知的项目与上面扫描的项目进行协调
            if (mSingleFile && mScannedIds.size() == 1) {
                // We can safely skip this step if the scan targeted a single
                // file which we scanned above
            } else {
                reconcileAndClean();
            }

            // Third, resolve any playlists that we scanned
  			//解析扫描的所有列表，并存入数据库
            resolvePlaylists();

            if (!mSingleFile) {
                final long durationMillis = SystemClock.elapsedRealtime() - startTime;
                Metrics.logScan(mVolumeName, mReason, mFileCount, durationMillis,
                        mInsertCount, mUpdateCount, mDeleteCount);
                if (ESWIN_AOSP_ENHANCEMENG_ENABLE) {
                    Log.d(TAG, "Scanned folders: " + mDirCount
                        + " videos: " + mVideoCount
                        + " audios: " + mAudioCount
                        + " images: " + mImageCount);
                }
            }
        }
```

接下来看一下walkFileTree的方法：

```java
        private void walkFileTree() {
            mSignal.throwIfCanceled();
          	//返回指定路径的父目录是否可扫描以及是否隐藏
            final Pair<Boolean, Boolean> isDirScannableAndHidden =
                    shouldScanPathAndIsPathHidden(mSingleFile ? mRoot.getParentFile() : mRoot);
            Log.d(TAG, "walkFileTree,path "+mRoot.getPath()+" depth: " + DEPTH_SIZE);
            if (isDirScannableAndHidden.first) {
                // This directory is scannable.
                Trace.beginSection("walkFileTree");

                if (isDirScannableAndHidden.second) {
                    // This directory is hidden
                    mHiddenDirCount++;
                }
                if (mSingleFile) {
                    acquireDirectoryLock(mRoot.getParentFile().toPath());
                }
                try {
                  	//客户定制
                    if (ESWIN_AOSP_ENHANCEMENG_ENABLE) {
                        Log.d(TAG, "walkFileTree, depth: " + DEPTH_SIZE);
                      //从指定目录扫描,第四个参数控制访问的行为
                    Files.walkFileTree(mRoot.toPath(),
                                EnumSet.noneOf(FileVisitOption.class),
                                DEPTH_SIZE,
                                this);
                    }
                  	//添加扫描的id
                    applyPending();
                } catch (IOException e) {
                    // This should never happen, so yell loudly
                    throw new IllegalStateException(e);
                } finally {
                    if (mSingleFile) {
                        releaseDirectoryLock(mRoot.getParentFile().toPath());
                    }
                    Trace.endSection();
                }
            }
        }
```



### 补充

#### MediaStore

MediaStore是Android系统提供的多媒体数据库，专门用于存放多媒体信息，通过ContentResolver对数据库进行操作。